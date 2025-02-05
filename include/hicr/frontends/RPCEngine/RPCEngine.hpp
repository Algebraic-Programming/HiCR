/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file RPCEngine.hpp
 * @brief Provides functionality for sending, listening to, and executing RPCs among HiCR Instances
 * @author S. Martin
 * @date 5/02/2025
 */

#pragma once

#include <hicr/core/L1/instanceManager.hpp>
#include <hicr/core/L1/communicationManager.hpp>
#include <hicr/core/L1/memoryManager.hpp>
#include <hicr/core/L1/topologyManager.hpp>
#include <hicr/frontends/channel/variableSize/mpsc/locking/consumer.hpp>
#include <hicr/frontends/channel/variableSize/mpsc/locking/producer.hpp>

#define _HICR_DEPLOYER_CHANNEL_PAYLOAD_CAPACITY 1048576
#define _HICR_DEPLOYER_CHANNEL_COUNT_CAPACITY 1024
#define _HICR_DEPLOYER_CHANNEL_BASE_TAG 0xF0000000
#define _HICR_DEPLOYER_CHANNEL_CONSUMER_SIZES_BUFFER_TAG _HICR_DEPLOYER_CHANNEL_BASE_TAG + 0
#define _HICR_DEPLOYER_CHANNEL_CONSUMER_PAYLOAD_BUFFER_TAG _HICR_DEPLOYER_CHANNEL_BASE_TAG + 1
#define _HICR_DEPLOYER_CHANNEL_CONSUMER_COORDINATION_BUFFER_SIZES_TAG _HICR_DEPLOYER_CHANNEL_BASE_TAG + 3
#define _HICR_DEPLOYER_CHANNEL_CONSUMER_COORDINATION_BUFFER_PAYLOADS_TAG _HICR_DEPLOYER_CHANNEL_BASE_TAG + 4
#define _HICR_DEPLOYER_CHANNEL_PRODUCER_COORDINATION_BUFFER_SIZES_TAG _HICR_DEPLOYER_CHANNEL_BASE_TAG + 5
#define _HICR_DEPLOYER_CHANNEL_PRODUCER_COORDINATION_BUFFER_PAYLOADS_TAG _HICR_DEPLOYER_CHANNEL_BASE_TAG + 6

namespace HiCR::frontend
{

class RPCEngine
{
  public:

  /**
   * Constructor
   *
   * @param[in] communicationManager The communication manager to use to communicate with other instances
   * @param[in] instanceManager The instance manager to use to get information about other instances
   * @param[in] memoryManager The memory manager to use to allocate buffer memory
   * @param[in] topologyManager The topology manager to use to discover memory spaces on which to allocate buffers
   */
  RPCEngine(L1::CommunicationManager        &communicationManager,
            L1::InstanceManager             &instanceManager,
            L1::MemoryManager               &memoryManager,
            L1::TopologyManager             &topologyManager,
            const uint64_t baseTag = _HICR_DEPLOYER_CHANNEL_BASE_TAG
            )
    : _communicationManager(communicationManager),
      _instanceManager(instanceManager),
      _memoryManager(memoryManager),
      _topologyManager(topologyManager),
      _baseTag(baseTag)
  {}

  __INLINE__ void initialize()
  {
    // Creating MPSC channels to receive RPC requests
  }


  /**
   * Default Destructor
   */
  ~RPCEngine() = default;

  private:

  __INLINE__ void initializeChannels()
  {
    // Getting my current instance
    const auto currentInstanceId = _instanceManager.getCurrentInstance()->getId();

    ////////// Creating consumer channel to receive variable sized messages from other instances

    // Gathering topology from the topology manager
    const auto t = _topologyManager.queryTopology();

    // Selecting first device
    auto d = *t.getDevices().begin();

    // Getting memory space list from device
    auto memSpaces = d->getMemorySpaceList();

    // Grabbing first memory space for buffering
    auto bufferMemorySpace = *memSpaces.begin();

    // Getting required buffer sizes
    auto tokenSizeBufferSize = HiCR::channel::variableSize::Base::getTokenBufferSize(sizeof(size_t), _HICR_DEPLOYER_CHANNEL_COUNT_CAPACITY);

    // Allocating token size buffer as a local memory slot
    auto tokenSizeBufferSlot = _memoryManager.allocateLocalMemorySlot(bufferMemorySpace, tokenSizeBufferSize);

    // Allocating token size buffer as a local memory slot
    auto payloadBufferSlot = _memoryManager.allocateLocalMemorySlot(bufferMemorySpace, _HICR_DEPLOYER_CHANNEL_PAYLOAD_CAPACITY);

    // Getting required buffer size for coordination buffers
    auto coordinationBufferSize = HiCR::channel::variableSize::Base::getCoordinationBufferSize();

    // Allocating coordination buffers
    auto localConsumerCoordinationBufferMessageSizes    = _memoryManager.allocateLocalMemorySlot(bufferMemorySpace, coordinationBufferSize);
    auto localConsumerCoordinationBufferMessagePayloads = _memoryManager.allocateLocalMemorySlot(bufferMemorySpace, coordinationBufferSize);

    // Initializing coordination buffers
    HiCR::channel::variableSize::Base::initializeCoordinationBuffer(localConsumerCoordinationBufferMessageSizes);
    HiCR::channel::variableSize::Base::initializeCoordinationBuffer(localConsumerCoordinationBufferMessagePayloads);

    // Exchanging local memory slots to become global for them to be used by the remote end
    _communicationManager.exchangeGlobalMemorySlots(_HICR_DEPLOYER_CHANNEL_CONSUMER_SIZES_BUFFER_TAG, {{currentInstanceId, tokenSizeBufferSlot}});
    _communicationManager.fence(_HICR_DEPLOYER_CHANNEL_CONSUMER_SIZES_BUFFER_TAG);

    _communicationManager.exchangeGlobalMemorySlots(_HICR_DEPLOYER_CHANNEL_CONSUMER_PAYLOAD_BUFFER_TAG, {{currentInstanceId, payloadBufferSlot}});
    _communicationManager.fence(_HICR_DEPLOYER_CHANNEL_CONSUMER_PAYLOAD_BUFFER_TAG);

    _communicationManager.exchangeGlobalMemorySlots(_HICR_DEPLOYER_CHANNEL_CONSUMER_COORDINATION_BUFFER_SIZES_TAG,
                                                    {{currentInstanceId, localConsumerCoordinationBufferMessageSizes}});
    _communicationManager.fence(_HICR_DEPLOYER_CHANNEL_CONSUMER_COORDINATION_BUFFER_SIZES_TAG);

    _communicationManager.exchangeGlobalMemorySlots(_HICR_DEPLOYER_CHANNEL_CONSUMER_COORDINATION_BUFFER_PAYLOADS_TAG,
                                                    {{currentInstanceId, localConsumerCoordinationBufferMessagePayloads}});
    _communicationManager.fence(_HICR_DEPLOYER_CHANNEL_CONSUMER_COORDINATION_BUFFER_PAYLOADS_TAG);

    // Obtaining the globally exchanged memory slots
    auto consumerMessagePayloadBuffer      = _communicationManager.getGlobalMemorySlot(_HICR_DEPLOYER_CHANNEL_CONSUMER_PAYLOAD_BUFFER_TAG, currentInstanceId);
    auto consumerMessageSizesBuffer        = _communicationManager.getGlobalMemorySlot(_HICR_DEPLOYER_CHANNEL_CONSUMER_SIZES_BUFFER_TAG, currentInstanceId);
    auto consumerCoodinationPayloadsBuffer = _communicationManager.getGlobalMemorySlot(_HICR_DEPLOYER_CHANNEL_CONSUMER_COORDINATION_BUFFER_PAYLOADS_TAG, currentInstanceId);
    auto consumerCoordinationSizesBuffer   = _communicationManager.getGlobalMemorySlot(_HICR_DEPLOYER_CHANNEL_CONSUMER_COORDINATION_BUFFER_SIZES_TAG, currentInstanceId);

    // Creating channel
    _consumerChannel = std::make_shared<HiCR::channel::variableSize::MPSC::locking::Consumer>(_communicationManager,
                                                                                              consumerMessagePayloadBuffer,
                                                                                              consumerMessageSizesBuffer,
                                                                                              localConsumerCoordinationBufferMessageSizes,
                                                                                              localConsumerCoordinationBufferMessagePayloads,
                                                                                              consumerCoordinationSizesBuffer,
                                                                                              consumerCoodinationPayloadsBuffer,
                                                                                              _HICR_DEPLOYER_CHANNEL_PAYLOAD_CAPACITY,
                                                                                              _HICR_DEPLOYER_CHANNEL_COUNT_CAPACITY);

    // Creating producer channels
    for (const auto &instance : _instanceManager.getInstances())
    {
      // Getting consumer instance id
      const auto consumerInstanceId = instance->getId();

      // Allocating coordination buffers
      auto localProducerSizeInfoBuffer                    = _memoryManager.allocateLocalMemorySlot(bufferMemorySpace, sizeof(size_t));
      auto localProducerCoordinationBufferMessageSizes    = _memoryManager.allocateLocalMemorySlot(bufferMemorySpace, coordinationBufferSize);
      auto localProducerCoordinationBufferMessagePayloads = _memoryManager.allocateLocalMemorySlot(bufferMemorySpace, coordinationBufferSize);

      // Initializing coordination buffers
      HiCR::channel::variableSize::Base::initializeCoordinationBuffer(localProducerCoordinationBufferMessageSizes);
      HiCR::channel::variableSize::Base::initializeCoordinationBuffer(localProducerCoordinationBufferMessagePayloads);

      // Obtaining the globally exchanged memory slots
      auto consumerMessagePayloadBuffer  = _communicationManager.getGlobalMemorySlot(_HICR_DEPLOYER_CHANNEL_CONSUMER_PAYLOAD_BUFFER_TAG, consumerInstanceId);
      auto consumerMessageSizesBuffer    = _communicationManager.getGlobalMemorySlot(_HICR_DEPLOYER_CHANNEL_CONSUMER_SIZES_BUFFER_TAG, consumerInstanceId);
      auto consumerPayloadConsumerBuffer = _communicationManager.getGlobalMemorySlot(_HICR_DEPLOYER_CHANNEL_CONSUMER_COORDINATION_BUFFER_PAYLOADS_TAG, consumerInstanceId);
      auto consumerSizesConsumerBuffer   = _communicationManager.getGlobalMemorySlot(_HICR_DEPLOYER_CHANNEL_CONSUMER_COORDINATION_BUFFER_SIZES_TAG, consumerInstanceId);

      // Creating channel
      _producerChannels[consumerInstanceId] = std::make_shared<HiCR::channel::variableSize::MPSC::locking::Producer>(_communicationManager,
                                                                                                                    localProducerSizeInfoBuffer,
                                                                                                                    consumerMessagePayloadBuffer,
                                                                                                                    consumerMessageSizesBuffer,
                                                                                                                    localProducerCoordinationBufferMessageSizes,
                                                                                                                    localProducerCoordinationBufferMessagePayloads,
                                                                                                                    consumerSizesConsumerBuffer,
                                                                                                                    consumerPayloadConsumerBuffer,
                                                                                                                    _HICR_DEPLOYER_CHANNEL_PAYLOAD_CAPACITY,
                                                                                                                    sizeof(uint8_t),
                                                                                                                    _HICR_DEPLOYER_CHANNEL_COUNT_CAPACITY);
    }
  }



  /**
   * The associated communication manager.
   */
  L1::CommunicationManager& _communicationManager;

  /**
   * The associated instance manager.
   */
  L1::InstanceManager& _instanceManager;

  /**
   * The associated memory manager.
   */
  L1::MemoryManager& _memoryManager;

  /**
   * The associated topology manager.
   */
  L1::TopologyManager& _topologyManager;

  const uint64_t _baseTag;

   /**
   * Consumer channels for receiving RPC messages from all other instances
   */
  std::shared_ptr<HiCR::channel::variableSize::MPSC::locking::Consumer> _consumerChannel;

  /**
   * Producer channels for sending RPC messages to all other instances
   */
  std::map<HiCR::L0::Instance::instanceId_t, std::shared_ptr<HiCR::channel::variableSize::MPSC::locking::Producer>> _producerChannels;

};

} // namespace HiCR::objectStore
