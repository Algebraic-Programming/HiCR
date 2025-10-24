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
    auto payloadBufferSlot =
      payloadMemoryManager.allocateLocalMemorySlot(payloadMemorySpace, HiCR::channel::variableSize::SPSC::Consumer::getPayloadBufferSize(CHANNEL_CAPACITY * sizeof(ELEMENT_TYPE)));

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

  std::unique_ptr<HiCR::CommunicationManager>              communicationManager;
  std::unique_ptr<HiCR::InstanceManager>                   instanceManager;
  std::unique_ptr<HiCR::MemoryManager>                     memoryManager;
  std::unique_ptr<HiCR::TopologyManager>                   topologyManager;
  std::unique_ptr<HiCR::backend::pthreads::ComputeManager> computeManager;

  std::unique_ptr<HiCR::channel::variableSize::SPSC::Consumer> consumer;
  std::unique_ptr<HiCR::channel::variableSize::SPSC::Producer> producer;

  std::shared_ptr<HiCR::MemorySpace> memorySpace;

  protected:

  void SetUp() override
  {
    instanceManager = std::make_unique<HiCR::backend::mpi::InstanceManager>(MPI_COMM_WORLD);

    // Sanity Check
    if (instanceManager->getInstances().size() != 2)
    {
      if (instanceManager->getCurrentInstance()->isRootInstance()) fprintf(stderr, "Launch error: MPI process count must be equal to 2\n");
      MPI_Finalize();
    }

    communicationManager = std::make_unique<HiCR::backend::mpi::CommunicationManager>(MPI_COMM_WORLD);
    memoryManager        = std::make_unique<HiCR::backend::mpi::MemoryManager>();
    computeManager       = std::make_unique<HiCR::backend::pthreads::ComputeManager>();
    topologyManager      = HiCR::backend::hwloc::TopologyManager::createDefault();

    _topology   = topologyManager->queryTopology();
    memorySpace = _topology.getDevices().begin().operator*()->getMemorySpaceList().begin().operator*();
  }

  void TearDown() override
  {
    for (auto &g : _globalSlots) { communicationManager->deregisterGlobalMemorySlot(g); }
    for (auto &g : _globalSlotsToDestroy) { communicationManager->destroyGlobalMemorySlot(g); }
    communicationManager->fence(CHANNEL_TAG);
    for (auto &l : _localSlots) { memoryManager->freeLocalMemorySlot(l); }
  }

  private:

  HiCR::Topology _topology;

  std::unordered_set<std::shared_ptr<HiCR::GlobalMemorySlot>> _globalSlots;
  std::unordered_set<std::shared_ptr<HiCR::GlobalMemorySlot>> _globalSlotsToDestroy;
  std::unordered_set<std::shared_ptr<HiCR::LocalMemorySlot>>  _localSlots;
};