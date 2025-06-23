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

/**
 * @file mocr.hpp
 * @brief Provides mock classes for HiCR's Manager classes for testing purposes
 * @author O. Korakitis
 * @date 03/10/2024
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <hicr/core/communicationManager.hpp>
#include <hicr/core/computeManager.hpp>
#include <hicr/core/instanceManager.hpp>
#include <hicr/core/memoryManager.hpp>
#include <hicr/core/topologyManager.hpp>

#include <hicr/backends/hwloc/instanceManager.hpp>

using namespace testing;

/* MoCR mocks HiCR :) */

/* Mock classes for dependencies. Currently, only the Manager classes are mocked. */

class MockCommunicationManager : public HiCR::CommunicationManager
{
  public:

  MOCK_METHOD(void, exchangeGlobalMemorySlotsImpl, (const HiCR::GlobalMemorySlot::tag_t, const std::vector<globalKeyMemorySlotPair_t> &), (override));
  MOCK_METHOD(void, destroyGlobalMemorySlotImpl, (std::shared_ptr<HiCR::GlobalMemorySlot>), (override));
  MOCK_METHOD(std::shared_ptr<HiCR::GlobalMemorySlot>, getGlobalMemorySlotImpl, (const HiCR::GlobalMemorySlot::tag_t, const HiCR::GlobalMemorySlot::globalKey_t), (override));
  MOCK_METHOD(void, queryMemorySlotUpdatesImpl, (std::shared_ptr<HiCR::LocalMemorySlot>), (override));
  MOCK_METHOD(void, fenceImpl, (HiCR::GlobalMemorySlot::tag_t), (override));
  MOCK_METHOD(bool, acquireGlobalLockImpl, (std::shared_ptr<HiCR::GlobalMemorySlot>), (override));
  MOCK_METHOD(void, releaseGlobalLockImpl, (std::shared_ptr<HiCR::GlobalMemorySlot>), (override));
  MOCK_METHOD(uint8_t *, serializeGlobalMemorySlot, (const std::shared_ptr<HiCR::GlobalMemorySlot> &), (const, override));
  MOCK_METHOD(std::shared_ptr<HiCR::GlobalMemorySlot>, deserializeGlobalMemorySlot, (uint8_t *, HiCR::GlobalMemorySlot::tag_t), (override));
  MOCK_METHOD(std::shared_ptr<HiCR::GlobalMemorySlot>, promoteLocalMemorySlot, (const std::shared_ptr<HiCR::LocalMemorySlot> &, HiCR::GlobalMemorySlot::tag_t), (override));
  MOCK_METHOD(void, destroyPromotedGlobalMemorySlot, (const std::shared_ptr<HiCR::GlobalMemorySlot> &), (override));

  /* Helper functions to expose protected methods */
  void registerGlobalMemorySlotPub(std::shared_ptr<HiCR::GlobalMemorySlot> memorySlot) { registerGlobalMemorySlot(memorySlot); }

  /*
   * Use a snippet like the following to customize the mock behavior: (place it in the SetUp method)
   *
   *   auto customExchangeGlobalMemorySlotsImpl = [this](const HiCR::GlobalMemorySlot::tag_t tag, const std::vector<HiCR::CommunicationManager::globalKeyMemorySlotPair_t> &memorySlots) {
   *     auto dummySlot = std::make_shared<HiCR::GlobalMemorySlot>(tag, 0, memorySlots[0].second);
   *     communicationManager.registerGlobalMemorySlotPub(dummySlot);
   *   };
   *
   *   ON_CALL(communicationManager, exchangeGlobalMemorySlotsImpl(_, _)).WillByDefault(Invoke(customExchangeGlobalMemorySlotsImpl));
   */
};

class MockMemoryManager : public HiCR::MemoryManager
{
  public:

  MOCK_METHOD((std::shared_ptr<HiCR::LocalMemorySlot>), allocateLocalMemorySlotImpl, (std::shared_ptr<HiCR::MemorySpace>, const size_t), (override));
  MOCK_METHOD((std::shared_ptr<HiCR::LocalMemorySlot>), registerLocalMemorySlotImpl, (std::shared_ptr<HiCR::MemorySpace>, void *const, const size_t), (override));
  MOCK_METHOD(void, freeLocalMemorySlotImpl, (std::shared_ptr<HiCR::LocalMemorySlot>), (override));
  MOCK_METHOD(void, deregisterLocalMemorySlotImpl, (std::shared_ptr<HiCR::LocalMemorySlot>), (override));

  /*
   * Use a snippet like the following to customize the mock behavior: (place it in the SetUp method)
   *
   *   auto customAllocateLocalMemorySlotImpl = [this](std::shared_ptr<HiCR::MemorySpace> memorySpace, const size_t size) {
   *     void *ptr = malloc(size);
   *     return std::make_shared<HiCR::LocalMemorySlot>(ptr, size, memorySpace);
   *   };
   *
   *   ON_CALL(memoryManager, allocateLocalMemorySlotImpl(_, _)).WillByDefault(Invoke(customAllocateLocalMemorySlotImpl));
   */
};

class MockMemorySpace : public HiCR::MemorySpace
{
  public:

  MOCK_METHOD(std::string, getType, (), (const, override));
  MOCK_METHOD(void, serializeImpl, (nlohmann::json &), (const, override));
  MOCK_METHOD(void, deserializeImpl, (const nlohmann::json &), (override));

  MockMemorySpace(const size_t size)
    : HiCR::MemorySpace(size)
  {
    ON_CALL(*this, getType()).WillByDefault(Return("MockMemorySpace"));
  }
};

class MockInstanceManager : public HiCR::InstanceManager
{
  public:

  MOCK_METHOD(void, finalize, (), (override));
  MOCK_METHOD(void, abort, (int), (override));
  MOCK_METHOD(HiCR::Instance::instanceId_t, getRootInstanceId, (), (const, override));

  MockInstanceManager()
  {
    ON_CALL(*this, getRootInstanceId()).WillByDefault(Return(0));
    auto instance = std::make_shared<HiCR::backend::hwloc::Instance>();
    setCurrentInstance(instance);
    addInstance(instance);
  }
};

class MockComputeManager : public HiCR::ComputeManager
{
  public:

  MOCK_METHOD((std::unique_ptr<HiCR::ProcessingUnit>), createProcessingUnit, (std::shared_ptr<HiCR::ComputeResource>), (const, override));
  MOCK_METHOD((std::unique_ptr<HiCR::ExecutionState>), createExecutionState, (std::shared_ptr<HiCR::ExecutionUnit>, void *const), (const, override));
};

class MockTopologyManager : public HiCR::TopologyManager
{
  public:

  MOCK_METHOD(HiCR::Topology, queryTopology, (), (override));
  MOCK_METHOD(HiCR::Topology, _deserializeTopology, (const nlohmann::json &), (const, override));
};
