/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file mocr.hpp
 * @brief Provides mock classes for HiCR's (L1) classes for testing purposes
 * @author O. Korakitis
 * @date 03/10/2024
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <hicr/core/L1/communicationManager.hpp>
#include <hicr/core/L1/computeManager.hpp>
#include <hicr/core/L1/instanceManager.hpp>
#include <hicr/core/L1/memoryManager.hpp>
#include <hicr/core/L1/topologyManager.hpp>

#include <hicr/backends/hwloc/L1/instanceManager.hpp>

using namespace testing;

/* MoCR mocks HiCR :) */

/* Mock classes for dependencies. Currently, only the higher level (L1) classes are mocked. */

class MockCommunicationManager : public HiCR::L1::CommunicationManager
{
  public:

  MOCK_METHOD(void, exchangeGlobalMemorySlotsImpl, (const HiCR::L0::GlobalMemorySlot::tag_t, const std::vector<globalKeyMemorySlotPair_t> &), (override));
  MOCK_METHOD(void, destroyGlobalMemorySlotImpl, (std::shared_ptr<HiCR::L0::GlobalMemorySlot>), (override));
  MOCK_METHOD(std::shared_ptr<HiCR::L0::GlobalMemorySlot>,
              getGlobalMemorySlotImpl,
              (const HiCR::L0::GlobalMemorySlot::tag_t, const HiCR::L0::GlobalMemorySlot::globalKey_t),
              (override));
  MOCK_METHOD(void, queryMemorySlotUpdatesImpl, (std::shared_ptr<HiCR::L0::LocalMemorySlot>), (override));
  MOCK_METHOD(void, fenceImpl, (HiCR::L0::GlobalMemorySlot::tag_t), (override));
  MOCK_METHOD(bool, acquireGlobalLockImpl, (std::shared_ptr<HiCR::L0::GlobalMemorySlot>), (override));
  MOCK_METHOD(void, releaseGlobalLockImpl, (std::shared_ptr<HiCR::L0::GlobalMemorySlot>), (override));
  MOCK_METHOD(uint8_t *, serializeGlobalMemorySlot, (const std::shared_ptr<HiCR::L0::GlobalMemorySlot> &), (const, override));
  MOCK_METHOD(std::shared_ptr<HiCR::L0::GlobalMemorySlot>, deserializeGlobalMemorySlot, (uint8_t *, HiCR::L0::GlobalMemorySlot::tag_t), (override));
  MOCK_METHOD(std::shared_ptr<HiCR::L0::GlobalMemorySlot>,
              promoteLocalMemorySlot,
              (const std::shared_ptr<HiCR::L0::LocalMemorySlot> &, HiCR::L0::GlobalMemorySlot::tag_t),
              (override));
  MOCK_METHOD(void, destroyPromotedGlobalMemorySlot, (const std::shared_ptr<HiCR::L0::GlobalMemorySlot> &), (override));

  /* Helper functions to expose protected methods */
  void registerGlobalMemorySlotPub(std::shared_ptr<HiCR::L0::GlobalMemorySlot> memorySlot) { registerGlobalMemorySlot(memorySlot); }

  /*
   * Use a snippet like the following to customize the mock behavior: (place it in the SetUp method)
   *
   *   auto customExchangeGlobalMemorySlotsImpl = [this](const HiCR::L0::GlobalMemorySlot::tag_t tag, const std::vector<HiCR::L1::CommunicationManager::globalKeyMemorySlotPair_t> &memorySlots) {
   *     auto dummySlot = std::make_shared<HiCR::L0::GlobalMemorySlot>(tag, 0, memorySlots[0].second);
   *     communicationManager.registerGlobalMemorySlotPub(dummySlot);
   *   };
   *
   *   ON_CALL(communicationManager, exchangeGlobalMemorySlotsImpl(_, _)).WillByDefault(Invoke(customExchangeGlobalMemorySlotsImpl));
   */
};

class MockMemoryManager : public HiCR::L1::MemoryManager
{
  public:

  MOCK_METHOD((std::shared_ptr<HiCR::L0::LocalMemorySlot>), allocateLocalMemorySlotImpl, (std::shared_ptr<HiCR::L0::MemorySpace>, const size_t), (override));
  MOCK_METHOD((std::shared_ptr<HiCR::L0::LocalMemorySlot>), registerLocalMemorySlotImpl, (std::shared_ptr<HiCR::L0::MemorySpace>, void *const, const size_t), (override));
  MOCK_METHOD(void, freeLocalMemorySlotImpl, (std::shared_ptr<HiCR::L0::LocalMemorySlot>), (override));
  MOCK_METHOD(void, deregisterLocalMemorySlotImpl, (std::shared_ptr<HiCR::L0::LocalMemorySlot>), (override));

  /*
   * Use a snippet like the following to customize the mock behavior: (place it in the SetUp method)
   *
   *   auto customAllocateLocalMemorySlotImpl = [this](std::shared_ptr<HiCR::L0::MemorySpace> memorySpace, const size_t size) {
   *     void *ptr = malloc(size);
   *     return std::make_shared<HiCR::L0::LocalMemorySlot>(ptr, size, memorySpace);
   *   };
   *
   *   ON_CALL(memoryManager, allocateLocalMemorySlotImpl(_, _)).WillByDefault(Invoke(customAllocateLocalMemorySlotImpl));
   */
};

class MockMemorySpace : public HiCR::L0::MemorySpace
{
  public:

  MOCK_METHOD(std::string, getType, (), (const, override));
  MOCK_METHOD(void, serializeImpl, (nlohmann::json &), (const, override));
  MOCK_METHOD(void, deserializeImpl, (const nlohmann::json &), (override));

  MockMemorySpace(const size_t size)
    : HiCR::L0::MemorySpace(size)
  {
    ON_CALL(*this, getType()).WillByDefault(Return("MockMemorySpace"));
  }
};

class MockInstanceManager : public HiCR::L1::InstanceManager
{
  public:

  MOCK_METHOD(void, launchRPC, (HiCR::L0::Instance &, const std::string &), (const, override));
  MOCK_METHOD(void, finalize, (), (override));
  MOCK_METHOD(void, abort, (int), (override));
  MOCK_METHOD(HiCR::L0::Instance::instanceId_t, getRootInstanceId, (), (const, override));
  MOCK_METHOD(std::shared_ptr<HiCR::L0::Instance>, createInstanceImpl, (const std::shared_ptr<HiCR::L0::InstanceTemplate> &), (override));
  MOCK_METHOD(std::shared_ptr<HiCR::L0::Instance>, addInstanceImpl, (HiCR::L0::Instance::instanceId_t), (override));
  MOCK_METHOD(void *, getReturnValueImpl, (HiCR::L0::Instance &), (const, override));
  MOCK_METHOD(void, submitReturnValueImpl, (const void *, const size_t), (override));
  MOCK_METHOD(void, listenImpl, (), (override));

  MockInstanceManager()
  {
    ON_CALL(*this, getRootInstanceId()).WillByDefault(Return(0));
    auto instance = std::make_shared<HiCR::backend::hwloc::L0::Instance>();
    setCurrentInstance(instance);
    addInstance(instance);
  }
};

class MockComputeManager : public HiCR::L1::ComputeManager
{
  public:

  MOCK_METHOD((std::unique_ptr<HiCR::L0::ProcessingUnit>), createProcessingUnit, (std::shared_ptr<HiCR::L0::ComputeResource>), (const, override));
  MOCK_METHOD((std::unique_ptr<HiCR::L0::ExecutionState>), createExecutionState, (std::shared_ptr<HiCR::L0::ExecutionUnit>, void *const), (const, override));
};

class MockTopologyManager : public HiCR::L1::TopologyManager
{
  public:

  MOCK_METHOD(HiCR::L0::Topology, queryTopology, (), (override));
  MOCK_METHOD(HiCR::L0::Topology, _deserializeTopology, (const nlohmann::json &), (const, override));
};
