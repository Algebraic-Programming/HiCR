#pragma once

#include <gtest/gtest.h>

#include <hicr/backends/hwloc/topologyManager.hpp>
#include <hicr/backends/mpi/communicationManager.hpp>
#include <hicr/backends/mpi/instanceManager.hpp>
#include <hicr/backends/mpi/memoryManager.hpp>
#include <hicr/backends/pthreads/computeManager.hpp>
#include <hicr/frontends/channel/variableSize/spsc/consumer.hpp>
#include <hicr/frontends/channel/variableSize/spsc/producer.hpp>

#include "common.hpp"

class ChannelFixture : public ::testing::Test
{
  public:

  std::unique_ptr<HiCR::channel::variableSize::SPSC::Producer> createProducer(HiCR::MemoryManager               &coordinationMemoryManager,
                                                                              HiCR::MemoryManager               &payloadMemoryManager,
                                                                              HiCR::CommunicationManager        &coordinationCommunicationManager,
                                                                              HiCR::CommunicationManager        &payloadCommunicationManager,
                                                                              std::shared_ptr<HiCR::MemorySpace> coordinationMemorySpace,
                                                                              std::shared_ptr<HiCR::MemorySpace> payloadMemorySpace,
                                                                              const size_t                       channelCapacity)
  {
    // Getting required buffer size
    auto coordinationBufferSize = HiCR::channel::variableSize::Base::getCoordinationBufferSize();

    // Allocating sizes buffer as a local memory slot
    auto coordinationBufferForCounts = coordinationMemoryManager.allocateLocalMemorySlot(coordinationMemorySpace, coordinationBufferSize);

    auto coordinationBufferForPayloads = coordinationMemoryManager.allocateLocalMemorySlot(coordinationMemorySpace, coordinationBufferSize);

    auto sizeInfoBuffer = coordinationMemoryManager.allocateLocalMemorySlot(coordinationMemorySpace, sizeof(size_t));

    // Initializing coordination buffers for message sizes and payloads (sets to zero the counters)
    HiCR::channel::variableSize::Base::initializeCoordinationBuffer(coordinationBufferForCounts);
    HiCR::channel::variableSize::Base::initializeCoordinationBuffer(coordinationBufferForPayloads);

    // Exchanging local memory slots to become global for them to be used by the remote end
    coordinationCommunicationManager.exchangeGlobalMemorySlots(CHANNEL_TAG,                                                                /* global tag */
                                                               {{PRODUCER_COORDINATION_BUFFER_FOR_SIZES_KEY, coordinationBufferForCounts}, /* key-slot pairs */
                                                                {PRODUCER_COORDINATION_BUFFER_FOR_PAYLOADS_KEY, coordinationBufferForPayloads}});

    payloadCommunicationManager.exchangeGlobalMemorySlots(CHANNEL_TAG, {});

    // Synchronizing so that all actors have finished registering their global memory slots
    coordinationCommunicationManager.fence(CHANNEL_TAG);
    payloadCommunicationManager.fence(CHANNEL_TAG);

    // Obtaining the globally exchanged memory slots
    auto sizesBuffer                           = coordinationCommunicationManager.getGlobalMemorySlot(CHANNEL_TAG, SIZES_BUFFER_KEY);
    auto producerCoordinationBufferForCounts   = coordinationCommunicationManager.getGlobalMemorySlot(CHANNEL_TAG, PRODUCER_COORDINATION_BUFFER_FOR_SIZES_KEY);
    auto producerCoordinationBufferForPayloads = coordinationCommunicationManager.getGlobalMemorySlot(CHANNEL_TAG, PRODUCER_COORDINATION_BUFFER_FOR_PAYLOADS_KEY);
    auto consumerCoordinationBufferForCounts   = coordinationCommunicationManager.getGlobalMemorySlot(CHANNEL_TAG, CONSUMER_COORDINATION_BUFFER_FOR_SIZES_KEY);
    auto consumerCoordinationBufferForPayloads = coordinationCommunicationManager.getGlobalMemorySlot(CHANNEL_TAG, CONSUMER_COORDINATION_BUFFER_FOR_PAYLOADS_KEY);
    auto payloadBuffer                         = payloadCommunicationManager.getGlobalMemorySlot(CHANNEL_TAG, CONSUMER_PAYLOAD_KEY);

    _localSlots.insert(coordinationBufferForCounts);
    _localSlots.insert(coordinationBufferForPayloads);
    _localSlots.insert(sizeInfoBuffer);

    _globalSlotsToDestroy.insert(sizesBuffer);
    _globalSlotsToDestroy.insert(producerCoordinationBufferForCounts);
    _globalSlotsToDestroy.insert(producerCoordinationBufferForPayloads);

    _globalSlots.insert(sizesBuffer);
    _globalSlots.insert(producerCoordinationBufferForCounts);
    _globalSlots.insert(producerCoordinationBufferForPayloads);
    _globalSlots.insert(consumerCoordinationBufferForCounts);
    _globalSlots.insert(consumerCoordinationBufferForPayloads);
    _globalSlots.insert(payloadBuffer);

    // Creating producer and consumer channels
    return std::make_unique<HiCR::channel::variableSize::SPSC::Producer>(coordinationCommunicationManager,
                                                                         payloadCommunicationManager,
                                                                         sizeInfoBuffer,
                                                                         payloadBuffer,
                                                                         sizesBuffer,
                                                                         coordinationBufferForCounts,
                                                                         coordinationBufferForPayloads,
                                                                         consumerCoordinationBufferForCounts,
                                                                         consumerCoordinationBufferForPayloads,
                                                                         CHANNEL_CAPACITY * sizeof(ELEMENT_TYPE),
                                                                         sizeof(ELEMENT_TYPE),
                                                                         channelCapacity);
  }

  std::unique_ptr<HiCR::channel::variableSize::SPSC::Consumer> createConsumer(HiCR::MemoryManager               &coordinationMemoryManager,
                                                                              HiCR::MemoryManager               &payloadMemoryManager,
                                                                              HiCR::CommunicationManager        &coordinationCommunicationManager,
                                                                              HiCR::CommunicationManager        &payloadCommunicationManager,
                                                                              std::shared_ptr<HiCR::MemorySpace> coordinationMemorySpace,
                                                                              std::shared_ptr<HiCR::MemorySpace> payloadMemorySpace,
                                                                              const size_t                       channelCapacity)
  {
    // Getting required buffer sizes
    auto sizesBufferSize = HiCR::channel::variableSize::Base::getTokenBufferSize(sizeof(size_t), channelCapacity);

    // Allocating sizes buffer as a local memory slot
    auto sizesBufferSlot = coordinationMemoryManager.allocateLocalMemorySlot(coordinationMemorySpace, sizesBufferSize);

    // Allocating payload buffer as a local memory slot
    auto payloadBufferSlot = payloadMemoryManager.allocateLocalMemorySlot(payloadMemorySpace, 2 * CHANNEL_CAPACITY * sizeof(ELEMENT_TYPE));

    // Getting required buffer size
    auto coordinationBufferSize = HiCR::channel::variableSize::Base::getCoordinationBufferSize();

    // Allocating coordination buffer for internal message size metadata
    auto coordinationBufferForCounts = coordinationMemoryManager.allocateLocalMemorySlot(coordinationMemorySpace, coordinationBufferSize);

    // Allocating coordination buffer for internal payload metadata
    auto coordinationBufferForPayloads = coordinationMemoryManager.allocateLocalMemorySlot(coordinationMemorySpace, coordinationBufferSize);

    // Initializing coordination buffer (sets to zero the counters)
    HiCR::channel::variableSize::Base::initializeCoordinationBuffer(coordinationBufferForCounts);

    HiCR::channel::variableSize::Base::initializeCoordinationBuffer(coordinationBufferForPayloads);

    // Exchanging local memory slots to become global for them to be used by the remote end
    coordinationCommunicationManager.exchangeGlobalMemorySlots(CHANNEL_TAG,
                                                               {{SIZES_BUFFER_KEY, sizesBufferSlot},
                                                                {CONSUMER_COORDINATION_BUFFER_FOR_SIZES_KEY, coordinationBufferForCounts},
                                                                {CONSUMER_COORDINATION_BUFFER_FOR_PAYLOADS_KEY, coordinationBufferForPayloads}});

    payloadCommunicationManager.exchangeGlobalMemorySlots(CHANNEL_TAG, {{CONSUMER_PAYLOAD_KEY, payloadBufferSlot}});

    // Synchronizing so that all actors have finished registering their global memory slots
    coordinationCommunicationManager.fence(CHANNEL_TAG);
    payloadCommunicationManager.fence(CHANNEL_TAG);

    // Obtaining the globally exchanged memory slots
    std::shared_ptr<HiCR::GlobalMemorySlot> globalSizesBufferSlot = coordinationCommunicationManager.getGlobalMemorySlot(CHANNEL_TAG, SIZES_BUFFER_KEY);

    auto producerCoordinationBufferForCounts   = coordinationCommunicationManager.getGlobalMemorySlot(CHANNEL_TAG, PRODUCER_COORDINATION_BUFFER_FOR_SIZES_KEY);
    auto producerCoordinationBufferForPayloads = coordinationCommunicationManager.getGlobalMemorySlot(CHANNEL_TAG, PRODUCER_COORDINATION_BUFFER_FOR_PAYLOADS_KEY);
    auto consumerCoordinationBufferForCounts   = coordinationCommunicationManager.getGlobalMemorySlot(CHANNEL_TAG, CONSUMER_COORDINATION_BUFFER_FOR_SIZES_KEY);
    auto consumerCoordinationBufferForPayloads = coordinationCommunicationManager.getGlobalMemorySlot(CHANNEL_TAG, CONSUMER_COORDINATION_BUFFER_FOR_PAYLOADS_KEY);
    auto payloadBuffer                         = payloadCommunicationManager.getGlobalMemorySlot(CHANNEL_TAG, CONSUMER_PAYLOAD_KEY);

    _localSlots.insert(sizesBufferSlot);
    _localSlots.insert(payloadBufferSlot);
    _localSlots.insert(coordinationBufferForCounts);
    _localSlots.insert(coordinationBufferForPayloads);

    _globalSlotsToDestroy.insert(consumerCoordinationBufferForCounts);
    _globalSlotsToDestroy.insert(consumerCoordinationBufferForPayloads);
    _globalSlotsToDestroy.insert(payloadBuffer);

    _globalSlots.insert(globalSizesBufferSlot);
    _globalSlots.insert(producerCoordinationBufferForCounts);
    _globalSlots.insert(producerCoordinationBufferForPayloads);
    _globalSlots.insert(consumerCoordinationBufferForCounts);
    _globalSlots.insert(consumerCoordinationBufferForPayloads);

    // Creating producer and consumer channels
    return std::make_unique<HiCR::channel::variableSize::SPSC::Consumer>(coordinationCommunicationManager,
                                                                         payloadCommunicationManager,
                                                                         payloadBuffer /*payload buffer */,
                                                                         globalSizesBufferSlot,
                                                                         coordinationBufferForCounts,
                                                                         coordinationBufferForPayloads,
                                                                         producerCoordinationBufferForCounts,
                                                                         producerCoordinationBufferForPayloads,
                                                                         CHANNEL_CAPACITY * sizeof(ELEMENT_TYPE),
                                                                         channelCapacity);
  }

  std::unique_ptr<HiCR::CommunicationManager>              _communicationManager;
  std::unique_ptr<HiCR::InstanceManager>                   _instanceManager;
  std::unique_ptr<HiCR::MemoryManager>                     _memoryManager;
  std::unique_ptr<HiCR::TopologyManager>                   _topologyManager;
  std::unique_ptr<HiCR::backend::pthreads::ComputeManager> _computeManager;

  std::unique_ptr<HiCR::channel::variableSize::SPSC::Consumer> _consumer;
  std::unique_ptr<HiCR::channel::variableSize::SPSC::Producer> _producer;

  std::shared_ptr<HiCR::MemorySpace> _memorySpace;

  protected:

  void SetUp() override
  {
    _instanceManager = std::make_unique<HiCR::backend::mpi::InstanceManager>(MPI_COMM_WORLD);

    // Sanity Check
    if (_instanceManager->getInstances().size() != 2)
    {
      if (_instanceManager->getCurrentInstance()->isRootInstance()) fprintf(stderr, "Launch error: MPI process count must be equal to 2\n");
      MPI_Finalize();
    }

    _communicationManager = std::make_unique<HiCR::backend::mpi::CommunicationManager>(MPI_COMM_WORLD);
    _memoryManager        = std::make_unique<HiCR::backend::mpi::MemoryManager>();
    _computeManager       = std::make_unique<HiCR::backend::pthreads::ComputeManager>();
    _topologyManager      = HiCR::backend::hwloc::TopologyManager::createDefault();

    _topology    = _topologyManager->queryTopology();
    _memorySpace = _topology.getDevices().begin().operator*()->getMemorySpaceList().begin().operator*();

    if (_instanceManager->getCurrentInstance()->isRootInstance())
    {
      _producer = createProducer(*_memoryManager, *_memoryManager, *_communicationManager, *_communicationManager, _memorySpace, _memorySpace, CHANNEL_CAPACITY);
    }
    else { _consumer = createConsumer(*_memoryManager, *_memoryManager, *_communicationManager, *_communicationManager, _memorySpace, _memorySpace, CHANNEL_CAPACITY); }
  }

  void TearDown() override
  {
    for (auto &g : _globalSlots) { _communicationManager->deregisterGlobalMemorySlot(g); }
    for (auto &g : _globalSlotsToDestroy) { _communicationManager->destroyGlobalMemorySlot(g); }
    _communicationManager->fence(CHANNEL_TAG);
    for (auto &l : _localSlots) { _memoryManager->freeLocalMemorySlot(l); }
  }

  private:

  HiCR::Topology _topology;

  std::unordered_set<std::shared_ptr<HiCR::GlobalMemorySlot>> _globalSlots;
  std::unordered_set<std::shared_ptr<HiCR::GlobalMemorySlot>> _globalSlotsToDestroy;
  std::unordered_set<std::shared_ptr<HiCR::LocalMemorySlot>>  _localSlots;
};