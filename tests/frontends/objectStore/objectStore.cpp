/*
 *   Copyright 2025 Huawei Technologies Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <gtest/gtest.h>
#include <hicr/frontends/objectStore/objectStore.hpp>
#include <hicr/backends/pthreads/communicationManager.hpp>

#include <mocr.hpp>

using namespace HiCR::objectStore;
using namespace testing;

// Test fixture for ObjectStore
class ObjectStoreTest : public ::testing::Test
{
  protected:

  MockCommunicationManager         communicationManager;
  MockMemoryManager                memoryManager;
  std::shared_ptr<MockMemorySpace> memorySpace;
  HiCR::GlobalMemorySlot::tag_t    tag        = 0;
  HiCR::Instance::instanceId_t     instanceId = 0;

  void SetUp() override
  {
    memorySpace = std::make_shared<MockMemorySpace>(1024);

    auto customRegisterLocalMemorySlotImpl = [this](std::shared_ptr<HiCR::MemorySpace> memorySpace, void *const ptr, const size_t size) {
      return std::make_shared<HiCR::LocalMemorySlot>(ptr, size, memorySpace);
    };

    ON_CALL(memoryManager, registerLocalMemorySlotImpl(_, _, _)).WillByDefault(Invoke(customRegisterLocalMemorySlotImpl));

    auto customPromoteLocalMemorySlot = [this](std::shared_ptr<HiCR::LocalMemorySlot> slot, HiCR::GlobalMemorySlot::tag_t tag) {
      return std::make_shared<HiCR::GlobalMemorySlot>(tag, 0, slot);
    };

    ON_CALL(communicationManager, promoteLocalMemorySlot(_, _)).WillByDefault(Invoke(customPromoteLocalMemorySlot));
  }
};

// Test createObject method
TEST_F(ObjectStoreTest, CreateObjectTest)
{
  ObjectStore store(communicationManager, tag, memoryManager, memorySpace, instanceId);
  void       *ptr = malloc(1024);

  EXPECT_CALL(memoryManager, registerLocalMemorySlotImpl(_, ptr, 1024)).Times(1);
  auto block = store.createObject(ptr, 1024, 42ul);

  EXPECT_EQ(block.getInstanceId(), 0ul);
  EXPECT_EQ(block.getId(), 42ul);
  EXPECT_EQ(block.getLocalSlot().getPointer(), ptr);
}

// Test publish method
TEST_F(ObjectStoreTest, PublishTest)
{
  ObjectStore store(communicationManager, tag, memoryManager, memorySpace, instanceId);
  void       *ptr = malloc(1024);

  EXPECT_CALL(memoryManager, registerLocalMemorySlotImpl(_, ptr, 1024)).Times(1);
  auto block   = store.createObject(ptr, 1024, 42);
  auto dataObj = std::make_shared<DataObject>(block);
  EXPECT_CALL(communicationManager, promoteLocalMemorySlot(_, _)).Times(1);
  store.publish(dataObj);
}

// Test get method
TEST_F(ObjectStoreTest, GetTest)
{
  // We need a real communication manager for this test
  HiCR::backend::pthreads::CommunicationManager communicationManager;

  ObjectStore store(communicationManager, tag, memoryManager, memorySpace, instanceId);
  char        data[8] = "test 12";
  auto        slot    = std::make_shared<HiCR::LocalMemorySlot>(&data, 8, memorySpace);
  auto        block   = store.createObject(slot, 0);
  auto        dataObj = std::make_shared<DataObject>(block);

  store.publish(dataObj);

  // The local slot allready exists, so we don't expect to allocate a new one; hence no test for that
  auto fetchedSlot = store.get(*dataObj);
  EXPECT_EQ(fetchedSlot->getSize(), 8ul);
  char *fetchedData = static_cast<char *>(fetchedSlot->getPointer());
  for (size_t i = 0; i < 8; i++) { EXPECT_EQ(fetchedData[i], data[i]); }
}

// Test destroy method
TEST_F(ObjectStoreTest, DestroyTest)
{
  ObjectStore store(communicationManager, tag, memoryManager, memorySpace, instanceId);
  char        data[8] = "test";

  EXPECT_CALL(memoryManager, registerLocalMemorySlotImpl(_, &data, 8)).Times(1);
  auto slot    = memoryManager.registerLocalMemorySlot(memorySpace, &data, 8);
  auto block   = store.createObject(slot, 0);
  auto dataObj = std::make_shared<DataObject>(block);

  EXPECT_CALL(communicationManager, promoteLocalMemorySlot(slot, tag)).Times(1);
  store.publish(dataObj);

  EXPECT_CALL(communicationManager, destroyPromotedGlobalMemorySlot(_)).Times(1);

  store.destroy(*dataObj);
}
