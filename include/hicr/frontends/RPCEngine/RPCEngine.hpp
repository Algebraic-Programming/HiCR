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
#include <hicr/frontends/channel/fixedSize/mpsc/nonlocking/consumer.hpp>
#include <hicr/frontends/channel/fixedSize/mpsc/nonlocking/producer.hpp>

/**
 * Maximum bytes to hold in capacity for return value channels
 */
#define _HICR_RPC_ENGINE_CHANNEL_PAYLOAD_CAPACITY 1048576

/**
 * Maximum number of messages to hold in capacity for RPC and return value channels
 */
#define _HICR_RPC_ENGINE_CHANNEL_COUNT_CAPACITY 1024

/**
 * Internal tag for RPC channels. Make sure you don't create other channels with the same tag
 */
#define _HICR_RPC_ENGINE_CHANNEL_BASE_TAG 0xF0000000

namespace HiCR::frontend
{

/**
 * This class encapsulates the HICR-based logic for sending, listening to, and executing remote procedure calls among HiCR distances.
 * An RPC request is represented by a single 64-bit identifier. The requester simply sends this identifier to the receiver instance.
 * Incoming requests are stored in a channel, ready to be picked up by the receiver. To pick up a request, the receiver must enter
 * the listening state. For an RPC to execute, the listener must have registered the corresponding index, together with an associated
 * listenable unit to run. The listenable unit is described as a HiCR execution unit object.
 */
class RPCEngine
{
  public:

  /**
   * Type definition for an index for a listenable unit.
   */
  using RPCTargetIndex_t = uint64_t;

  /**
   * Constructor
   *
   * @param[in] communicationManager The communication manager to use to communicate with other instances
   * @param[in] instanceManager The instance manager to use to get information about other instances
   * @param[in] memoryManager The memory manager to use to allocate buffer memory
   * @param[in] computeManager The compute manager to use to execute incoming RPCs
   * @param[in] bufferMemorySpace The memory space where the RPC engine will allocate all of its internal buffer from
   * @param[in] computeResource The compute resource to use to execute RPCs
   * @param[in] baseTag The tag to use for the creation of channels. Provide different values if you plan to create multiple RPC engines otherwise collisions might occur
   */
  RPCEngine(L1::CommunicationManager            &communicationManager,
            L1::InstanceManager                 &instanceManager,
            L1::MemoryManager                   &memoryManager,
            L1::ComputeManager                  &computeManager,
            std::shared_ptr<L0::MemorySpace>     bufferMemorySpace,
            std::shared_ptr<L0::ComputeResource> computeResource,
            const uint64_t                       baseTag = _HICR_RPC_ENGINE_CHANNEL_BASE_TAG)
    : _communicationManager(communicationManager),
      _instanceManager(instanceManager),
      _memoryManager(memoryManager),
      _computeManager(computeManager),
      _bufferMemorySpace(bufferMemorySpace),
      _computeResource(computeResource),
      _baseTag(baseTag)
  {}

  /**
   * Initializes the RPC engine:
   *   - Creates RPC channels
   *   - Creates Return value channels
   */
  __INLINE__ void initialize()
  {
    // Creating MPSC channels to receive RPC requests
    initializeRPCChannels();
    initializeReturnValueChannels();
  }

  /**
   * Default Destructor
   */
  ~RPCEngine() = default;

  /**
   * Function to add an RPC target with a name, and the combination of a execution unit and the processing unit that is in charge of executing it
   * \param[in] RPCName Name of the RPC to add
   * \param[in] e Indicates the execution unit to run when this RPC is triggered
   */
  __INLINE__ void addRPCTarget(const std::string &RPCName, const std::shared_ptr<HiCR::L0::ExecutionUnit> e)
  {
    // Obtaining hash from the RPC name
    const auto idx = getRPCTargetIndexFromString(RPCName);

    // Inserting the new entry
    _RPCTargetMap[idx] = e;
  }

  /**
   * Function to put the current instance to listen for incoming RPCs
   */
  __INLINE__ void listen()
  {
    // Calling the backend-specific implementation of the listen function
    while (_RPCConsumerChannel->getDepth() == 0) _RPCConsumerChannel->updateDepth();

    // Once a request has arrived, gather its value from the channel
    auto              request   = _RPCConsumerChannel->peek();
    auto              requester = request[0];
    RPCTargetIndex_t *buffer    = (RPCTargetIndex_t *)(_RPCConsumerChannel->getTokenBuffers()[requester]->getSourceLocalMemorySlot()->getPointer());
    auto              rpcIdx    = buffer[request[1]];
    _RPCConsumerChannel->pop();

    // Setting requester instance index
    _requesterInstanceIdx = requester;

    // Execute RPC
    executeRPC(rpcIdx);
  }

  /**
   * Function to request the execution of a remote function in a remote HiCR instance
   * \param[in] RPCName The name of the RPC to run
   * \param[in] instance Instance on which to run the RPC
   */
  virtual void requestRPC(HiCR::L0::Instance &instance, const std::string &RPCName)
  {
    const auto targetInstanceId = instance.getId();
    const auto targetRPCIdx     = getRPCTargetIndexFromString(RPCName);

    // Registering source buffer
    auto tempBufferSlot = _memoryManager.registerLocalMemorySlot(_bufferMemorySpace, (void *)&targetRPCIdx, sizeof(RPCTargetIndex_t));

    // Sending source buffer
    _RPCProducerChannels.at(targetInstanceId)->push(tempBufferSlot);
  }

  /**
   * Function to submit a return value for the currently running RPC
   * \param[in] pointer Pointer to the start of the data buffer to send
   * \param[in] size Size of the data buffer to send
   */
  __INLINE__ void submitReturnValue(void *pointer, const size_t size)
  {
    // Getting source buffers
    auto tempBufferSlot   = _memoryManager.allocateLocalMemorySlot(_bufferMemorySpace, size);
    auto sourceBufferSlot = _memoryManager.registerLocalMemorySlot(_bufferMemorySpace, pointer, size);

    // Copying data
    _communicationManager.memcpy(tempBufferSlot, 0, sourceBufferSlot, 0, size);

    // Waiting for communication to end
    _communicationManager.fence(tempBufferSlot, 1, 0);

    // Sending return value data
    _returnValueProducerChannels.at(_requesterInstanceIdx)->push(tempBufferSlot);

    // Freeing up local memory slot
    _memoryManager.freeLocalMemorySlot(tempBufferSlot);
  }

  /**
   * Function to get a return value from a remote instance that ran an RPC
   * \param[in] instance Instance from which to read the return value. An RPC request should be sent to that instance before calling this function.
   * \return A pointer to a newly allocated local memory slot containing the return value
   */
  __INLINE__ std::shared_ptr<HiCR::L0::LocalMemorySlot> getReturnValue(HiCR::L0::Instance &instance) const
  {
    // Calling the backend-specific implementation of the listen function
    while (_returnValueConsumerChannel->isEmpty())
      ;

    // Calling backend-specific implementation of this function
    auto returnValue = _returnValueConsumerChannel->peek();

    // Getting message info
    auto msgOffset = returnValue[0];
    auto msgSize   = returnValue[1];

    // Creating local buffer
    auto tempBufferSlot = _memoryManager.allocateLocalMemorySlot(_bufferMemorySpace, msgSize);

    // Copying data
    _communicationManager.memcpy(tempBufferSlot, 0, _returnValueConsumerChannel->getPayloadBufferMemorySlot(), msgOffset, msgSize);

    // Waiting for communication to end
    _communicationManager.fence(tempBufferSlot, 1, 0);

    // Freeing up channel
    _returnValueConsumerChannel->pop();

    // Returning internal buffer
    return tempBufferSlot;
  }

  /**
   * Gets the internal communication manager this module was initialized with
   * 
   * @return A pointer to the internal communication manager
   */
  [[nodiscard]] __INLINE__ HiCR::L1::CommunicationManager *getCommunicationManager() const { return &_communicationManager; }

  /**
   * Gets the internal instance manager this module was initialized with
   * 
   * @return A pointer to the internal instance manager
   */
  [[nodiscard]] __INLINE__ HiCR::L1::InstanceManager *getInstanceManager() const { return &_instanceManager; }

  /**
   * Gets the internal memory manager this module was initialized with
   * 
   * @return A pointer to the internal memory manager
   */
  [[nodiscard]] __INLINE__ HiCR::L1::MemoryManager *getMemoryManager() const { return &_memoryManager; }

  /**
   * Gets the internal compute manager this module was initialized with
   * 
   * @return A pointer to the internal computey manager
   */
  [[nodiscard]] __INLINE__ HiCR::L1::ComputeManager *getComputeManager() const { return &_computeManager; }

  private:

  /**
   * Generates a 64-bit hash value from a given string. Useful for compressing the name of RPCs
   *
   * @param[in] name A string (e.g., the name of an RPC to compress)
   * @return The 64-bit hashed value of the name provided
   */
  static RPCTargetIndex_t getRPCTargetIndexFromString(const std::string &name) { return std::hash<std::string>()(name); }

  /**
   * Internal function used to initiate the execution of the requested RPC
   * \param[in] rpcIdx Index to the RPC to run (hash to save overhead, the name is no longer recoverable)
   */
  __INLINE__ void executeRPC(const RPCTargetIndex_t rpcIdx) const
  {
    // Getting RPC target from the index
    if (_RPCTargetMap.contains(rpcIdx) == false) HICR_THROW_RUNTIME("Attempting to run an RPC target (Hash: %lu) that was not defined in this instance (0x%lX).\n", rpcIdx, this);
    auto e = _RPCTargetMap.at(rpcIdx);

    // Creating new processing unit to execute the RPC
    auto p = _computeManager.createProcessingUnit(_computeResource);
    _computeManager.initialize(p);

    // Creating execution state
    auto s = _computeManager.createExecutionState(e);

    // Executing RPC
    _computeManager.start(p, s);

    // Waiting for execution to finish
    _computeManager.await(p);
  }

  __INLINE__ void initializeRPCChannels()
  {
    // Defining tag values
    const uint64_t _HICR_RPC_ENGINE_CHANNEL_CONSUMER_TOKEN_BUFFER_TAG        = _baseTag + 4;
    const uint64_t _HICR_RPC_ENGINE_CHANNEL_PRODUCER_COORDINATION_BUFFER_TAG = _baseTag + 5;
    const uint64_t _HICR_RPC_ENGINE_CHANNEL_CONSUMER_COORDINATION_BUFFER_TAG = _baseTag + 6;

    // Getting required buffer sizes
    auto tokenSize = sizeof(RPCTargetIndex_t);

    // Getting required buffer sizes
    auto tokenBufferSize = HiCR::channel::fixedSize::Base::getTokenBufferSize(tokenSize, _HICR_RPC_ENGINE_CHANNEL_COUNT_CAPACITY);

    // Getting my current instance
    const auto currentInstanceId = _instanceManager.getCurrentInstance()->getId();

    // Getting total instance count
    const auto instanceCount = _instanceManager.getInstances().size();

    // Creating and exchanging buffers

    std::vector<HiCR::L1::CommunicationManager::globalKeyMemorySlotPair_t> tokenBuffers;
    std::vector<HiCR::L1::CommunicationManager::globalKeyMemorySlotPair_t> consumerCoordinationBuffers;
    std::vector<HiCR::L1::CommunicationManager::globalKeyMemorySlotPair_t> producerCoordinationBuffers;
    std::vector<std::shared_ptr<HiCR::L0::LocalMemorySlot>>                localConsumerCoordinationBuffers;
    std::vector<std::shared_ptr<HiCR::L0::LocalMemorySlot>>                localProducerCoordinationBuffers;

    auto coordinationBufferSize = HiCR::channel::fixedSize::Base::getCoordinationBufferSize();

    for (size_t i = 0; i < instanceCount; i++)
    {
      // Calculating these particular slots' key
      const HiCR::L0::GlobalMemorySlot::globalKey_t slotkey = currentInstanceId * instanceCount + i;

      // consumer needs to allocate #producers token buffers for #producers SPSCs
      auto tokenBufferSlot = _memoryManager.allocateLocalMemorySlot(_bufferMemorySpace, tokenBufferSize);

      // consumer needs to allocate #producers consumer side coordination buffers for #producers SPSCs
      auto consumerCoordinationBuffer = _memoryManager.allocateLocalMemorySlot(_bufferMemorySpace, coordinationBufferSize);
      HiCR::channel::fixedSize::Base::initializeCoordinationBuffer(consumerCoordinationBuffer);

      // producer needs to allocate #consumers producer-side coordination buffers for #consumer SPSCs
      auto producerCoordinationBuffer = _memoryManager.allocateLocalMemorySlot(_bufferMemorySpace, coordinationBufferSize);
      HiCR::channel::fixedSize::Base::initializeCoordinationBuffer(producerCoordinationBuffer);

      // Adding to collections
      localConsumerCoordinationBuffers.push_back(consumerCoordinationBuffer);
      localProducerCoordinationBuffers.push_back(producerCoordinationBuffer);
      tokenBuffers.push_back(std::make_pair(slotkey, tokenBufferSlot));
      consumerCoordinationBuffers.push_back(std::make_pair(slotkey, consumerCoordinationBuffer));
      producerCoordinationBuffers.push_back(std::make_pair(slotkey, producerCoordinationBuffer));
    }

    // communicate to producers the token buffer references
    _communicationManager.exchangeGlobalMemorySlots(_HICR_RPC_ENGINE_CHANNEL_CONSUMER_TOKEN_BUFFER_TAG, tokenBuffers);
    _communicationManager.fence(_HICR_RPC_ENGINE_CHANNEL_CONSUMER_TOKEN_BUFFER_TAG);

    // get from producers their coordination buffer references
    _communicationManager.exchangeGlobalMemorySlots(_HICR_RPC_ENGINE_CHANNEL_PRODUCER_COORDINATION_BUFFER_TAG, producerCoordinationBuffers);
    _communicationManager.fence(_HICR_RPC_ENGINE_CHANNEL_PRODUCER_COORDINATION_BUFFER_TAG);

    // communicate to producers the consumer buffers
    _communicationManager.exchangeGlobalMemorySlots(_HICR_RPC_ENGINE_CHANNEL_CONSUMER_COORDINATION_BUFFER_TAG, consumerCoordinationBuffers);
    _communicationManager.fence(_HICR_RPC_ENGINE_CHANNEL_CONSUMER_COORDINATION_BUFFER_TAG);

    ////////// Creating consumer channel to receive fixed sized RPC requests from other instances
    {
      std::vector<std::shared_ptr<HiCR::L0::GlobalMemorySlot>> globalConsumerTokenBuffers;
      std::vector<std::shared_ptr<HiCR::L0::GlobalMemorySlot>> globalConsumerCoordinationBuffers;
      std::vector<std::shared_ptr<HiCR::L0::GlobalMemorySlot>> globalProducerCoordinationBuffers;

      for (size_t i = 0; i < instanceCount; i++)
      {
        // Calculating these particular slots' key
        const HiCR::L0::GlobalMemorySlot::globalKey_t localSlotKey  = currentInstanceId * instanceCount + i;
        const HiCR::L0::GlobalMemorySlot::globalKey_t remoteSlotKey = i * instanceCount + currentInstanceId;

        auto globalConsumerTokenBufferSlot = _communicationManager.getGlobalMemorySlot(_HICR_RPC_ENGINE_CHANNEL_CONSUMER_TOKEN_BUFFER_TAG, localSlotKey);
        globalConsumerTokenBuffers.push_back(globalConsumerTokenBufferSlot);

        auto producerCoordinationBuffer = _communicationManager.getGlobalMemorySlot(_HICR_RPC_ENGINE_CHANNEL_PRODUCER_COORDINATION_BUFFER_TAG, remoteSlotKey);
        globalProducerCoordinationBuffers.push_back(producerCoordinationBuffer);

        auto consumerCoordinationBuffer = _communicationManager.getGlobalMemorySlot(_HICR_RPC_ENGINE_CHANNEL_CONSUMER_COORDINATION_BUFFER_TAG, localSlotKey);
        globalConsumerCoordinationBuffers.push_back(consumerCoordinationBuffer);
      }

      _RPCConsumerChannel = std::make_shared<HiCR::channel::fixedSize::MPSC::nonlocking::Consumer>(
        _communicationManager, globalConsumerTokenBuffers, localConsumerCoordinationBuffers, globalProducerCoordinationBuffers, tokenSize, _HICR_RPC_ENGINE_CHANNEL_COUNT_CAPACITY);
    }

    ////////// Creating producer channels to send fixed sized RPC requests to other instances

    {
      std::vector<std::shared_ptr<HiCR::L0::GlobalMemorySlot>> globalConsumerTokenBuffers;
      std::vector<std::shared_ptr<HiCR::L0::GlobalMemorySlot>> globalConsumerCoordinationBuffers;
      std::vector<std::shared_ptr<HiCR::L0::GlobalMemorySlot>> globalProducerCoordinationBuffers;

      for (size_t i = 0; i < instanceCount; i++)
      {
        // Calculating these particular slots' key
        const HiCR::L0::GlobalMemorySlot::globalKey_t localSlotKey  = currentInstanceId * instanceCount + i;
        const HiCR::L0::GlobalMemorySlot::globalKey_t remoteSlotKey = i * instanceCount + currentInstanceId;

        auto globalConsumerTokenBufferSlot = _communicationManager.getGlobalMemorySlot(_HICR_RPC_ENGINE_CHANNEL_CONSUMER_TOKEN_BUFFER_TAG, remoteSlotKey);
        globalConsumerTokenBuffers.push_back(globalConsumerTokenBufferSlot);

        auto producerCoordinationBuffer = _communicationManager.getGlobalMemorySlot(_HICR_RPC_ENGINE_CHANNEL_PRODUCER_COORDINATION_BUFFER_TAG, localSlotKey);
        globalProducerCoordinationBuffers.push_back(producerCoordinationBuffer);

        auto consumerCoordinationBuffer = _communicationManager.getGlobalMemorySlot(_HICR_RPC_ENGINE_CHANNEL_CONSUMER_COORDINATION_BUFFER_TAG, remoteSlotKey);
        globalConsumerCoordinationBuffers.push_back(consumerCoordinationBuffer);
      }

      for (size_t i = 0; i < instanceCount; i++)
      {
        // Getting consumer instance id
        const auto consumerInstanceId = _instanceManager.getInstances()[i]->getId();

        // Creating producer channel
        // This call does the same as the SPSC Producer constructor, as
        // the producer of MPSC::nonlocking has the same view
        _RPCProducerChannels[consumerInstanceId] =
          std::make_shared<HiCR::channel::fixedSize::MPSC::nonlocking::Producer>(_communicationManager,
                                                                                 globalConsumerTokenBuffers[i],
                                                                                 globalProducerCoordinationBuffers[i]->getSourceLocalMemorySlot(),
                                                                                 globalConsumerCoordinationBuffers[i],
                                                                                 tokenSize,
                                                                                 _HICR_RPC_ENGINE_CHANNEL_COUNT_CAPACITY);
      }
    }
  }

  __INLINE__ void initializeReturnValueChannels()
  {
    // Defining tag values
    const uint64_t _HICR_RPC_ENGINE_CHANNEL_CONSUMER_SIZES_BUFFER_TAG                 = _baseTag + 0;
    const uint64_t _HICR_RPC_ENGINE_CHANNEL_CONSUMER_PAYLOAD_BUFFER_TAG               = _baseTag + 1;
    const uint64_t _HICR_RPC_ENGINE_CHANNEL_CONSUMER_COORDINATION_BUFFER_SIZES_TAG    = _baseTag + 2;
    const uint64_t _HICR_RPC_ENGINE_CHANNEL_CONSUMER_COORDINATION_BUFFER_PAYLOADS_TAG = _baseTag + 3;

    // Getting my current instance
    const auto currentInstanceId = _instanceManager.getCurrentInstance()->getId();

    ////////// Creating consumer channel to receive variable sized messages from other instances

    // Getting required buffer sizes
    auto tokenSizeBufferSize = HiCR::channel::variableSize::Base::getTokenBufferSize(sizeof(size_t), _HICR_RPC_ENGINE_CHANNEL_COUNT_CAPACITY);

    // Allocating token size buffer as a local memory slot
    auto tokenSizeBufferSlot = _memoryManager.allocateLocalMemorySlot(_bufferMemorySpace, tokenSizeBufferSize);

    // Allocating token size buffer as a local memory slot
    auto payloadBufferSlot = _memoryManager.allocateLocalMemorySlot(_bufferMemorySpace, _HICR_RPC_ENGINE_CHANNEL_PAYLOAD_CAPACITY);

    // Getting required buffer size for coordination buffers
    auto coordinationBufferSize = HiCR::channel::variableSize::Base::getCoordinationBufferSize();

    // Allocating coordination buffers
    auto localConsumerCoordinationBufferMessageSizes    = _memoryManager.allocateLocalMemorySlot(_bufferMemorySpace, coordinationBufferSize);
    auto localConsumerCoordinationBufferMessagePayloads = _memoryManager.allocateLocalMemorySlot(_bufferMemorySpace, coordinationBufferSize);

    // Initializing coordination buffers
    HiCR::channel::variableSize::Base::initializeCoordinationBuffer(localConsumerCoordinationBufferMessageSizes);
    HiCR::channel::variableSize::Base::initializeCoordinationBuffer(localConsumerCoordinationBufferMessagePayloads);

    // Exchanging local memory slots to become global for them to be used by the remote end
    _communicationManager.exchangeGlobalMemorySlots(_HICR_RPC_ENGINE_CHANNEL_CONSUMER_SIZES_BUFFER_TAG, {{currentInstanceId, tokenSizeBufferSlot}});
    _communicationManager.fence(_HICR_RPC_ENGINE_CHANNEL_CONSUMER_SIZES_BUFFER_TAG);

    _communicationManager.exchangeGlobalMemorySlots(_HICR_RPC_ENGINE_CHANNEL_CONSUMER_PAYLOAD_BUFFER_TAG, {{currentInstanceId, payloadBufferSlot}});
    _communicationManager.fence(_HICR_RPC_ENGINE_CHANNEL_CONSUMER_PAYLOAD_BUFFER_TAG);

    _communicationManager.exchangeGlobalMemorySlots(_HICR_RPC_ENGINE_CHANNEL_CONSUMER_COORDINATION_BUFFER_SIZES_TAG,
                                                    {{currentInstanceId, localConsumerCoordinationBufferMessageSizes}});
    _communicationManager.fence(_HICR_RPC_ENGINE_CHANNEL_CONSUMER_COORDINATION_BUFFER_SIZES_TAG);

    _communicationManager.exchangeGlobalMemorySlots(_HICR_RPC_ENGINE_CHANNEL_CONSUMER_COORDINATION_BUFFER_PAYLOADS_TAG,
                                                    {{currentInstanceId, localConsumerCoordinationBufferMessagePayloads}});
    _communicationManager.fence(_HICR_RPC_ENGINE_CHANNEL_CONSUMER_COORDINATION_BUFFER_PAYLOADS_TAG);

    // Obtaining the globally exchanged memory slots
    auto consumerMessagePayloadBuffer      = _communicationManager.getGlobalMemorySlot(_HICR_RPC_ENGINE_CHANNEL_CONSUMER_PAYLOAD_BUFFER_TAG, currentInstanceId);
    auto consumerMessageSizesBuffer        = _communicationManager.getGlobalMemorySlot(_HICR_RPC_ENGINE_CHANNEL_CONSUMER_SIZES_BUFFER_TAG, currentInstanceId);
    auto consumerCoodinationPayloadsBuffer = _communicationManager.getGlobalMemorySlot(_HICR_RPC_ENGINE_CHANNEL_CONSUMER_COORDINATION_BUFFER_PAYLOADS_TAG, currentInstanceId);
    auto consumerCoordinationSizesBuffer   = _communicationManager.getGlobalMemorySlot(_HICR_RPC_ENGINE_CHANNEL_CONSUMER_COORDINATION_BUFFER_SIZES_TAG, currentInstanceId);

    // Creating channel
    _returnValueConsumerChannel = std::make_shared<HiCR::channel::variableSize::MPSC::locking::Consumer>(_communicationManager,
                                                                                                         consumerMessagePayloadBuffer,
                                                                                                         consumerMessageSizesBuffer,
                                                                                                         localConsumerCoordinationBufferMessageSizes,
                                                                                                         localConsumerCoordinationBufferMessagePayloads,
                                                                                                         consumerCoordinationSizesBuffer,
                                                                                                         consumerCoodinationPayloadsBuffer,
                                                                                                         _HICR_RPC_ENGINE_CHANNEL_PAYLOAD_CAPACITY,
                                                                                                         _HICR_RPC_ENGINE_CHANNEL_COUNT_CAPACITY);

    // Creating producer channels
    for (const auto &instance : _instanceManager.getInstances())
    {
      // Getting consumer instance id
      const auto consumerInstanceId = instance->getId();

      // Allocating coordination buffers
      auto localProducerSizeInfoBuffer                    = _memoryManager.allocateLocalMemorySlot(_bufferMemorySpace, sizeof(size_t));
      auto localProducerCoordinationBufferMessageSizes    = _memoryManager.allocateLocalMemorySlot(_bufferMemorySpace, coordinationBufferSize);
      auto localProducerCoordinationBufferMessagePayloads = _memoryManager.allocateLocalMemorySlot(_bufferMemorySpace, coordinationBufferSize);

      // Initializing coordination buffers
      HiCR::channel::variableSize::Base::initializeCoordinationBuffer(localProducerCoordinationBufferMessageSizes);
      HiCR::channel::variableSize::Base::initializeCoordinationBuffer(localProducerCoordinationBufferMessagePayloads);

      // Obtaining the globally exchanged memory slots
      auto consumerMessagePayloadBuffer  = _communicationManager.getGlobalMemorySlot(_HICR_RPC_ENGINE_CHANNEL_CONSUMER_PAYLOAD_BUFFER_TAG, consumerInstanceId);
      auto consumerMessageSizesBuffer    = _communicationManager.getGlobalMemorySlot(_HICR_RPC_ENGINE_CHANNEL_CONSUMER_SIZES_BUFFER_TAG, consumerInstanceId);
      auto consumerPayloadConsumerBuffer = _communicationManager.getGlobalMemorySlot(_HICR_RPC_ENGINE_CHANNEL_CONSUMER_COORDINATION_BUFFER_PAYLOADS_TAG, consumerInstanceId);
      auto consumerSizesConsumerBuffer   = _communicationManager.getGlobalMemorySlot(_HICR_RPC_ENGINE_CHANNEL_CONSUMER_COORDINATION_BUFFER_SIZES_TAG, consumerInstanceId);

      // Creating channel
      _returnValueProducerChannels[consumerInstanceId] = std::make_shared<HiCR::channel::variableSize::MPSC::locking::Producer>(_communicationManager,
                                                                                                                                localProducerSizeInfoBuffer,
                                                                                                                                consumerMessagePayloadBuffer,
                                                                                                                                consumerMessageSizesBuffer,
                                                                                                                                localProducerCoordinationBufferMessageSizes,
                                                                                                                                localProducerCoordinationBufferMessagePayloads,
                                                                                                                                consumerSizesConsumerBuffer,
                                                                                                                                consumerPayloadConsumerBuffer,
                                                                                                                                _HICR_RPC_ENGINE_CHANNEL_PAYLOAD_CAPACITY,
                                                                                                                                sizeof(uint8_t),
                                                                                                                                _HICR_RPC_ENGINE_CHANNEL_COUNT_CAPACITY);
    }
  }

  /**
   * The associated communication manager.
   */
  L1::CommunicationManager &_communicationManager;

  /**
   * The associated instance manager.
   */
  L1::InstanceManager &_instanceManager;

  /**
   * The associated memory manager.
   */
  L1::MemoryManager &_memoryManager;

  /**
   * The associated compute manager.
   */
  L1::ComputeManager &_computeManager;

  /**
   * Memory space to use for all buffering
   */
  const std::shared_ptr<HiCR::L0::MemorySpace> _bufferMemorySpace;

  /**
   * The compute resource to use for executing RPCs
   */
  const std::shared_ptr<HiCR::L0::ComputeResource> _computeResource;

  const uint64_t _baseTag;

  size_t _requesterInstanceIdx;

  /**
   * Consumer channels for receiving return value messages from all other instances
   */
  std::shared_ptr<HiCR::channel::variableSize::MPSC::locking::Consumer> _returnValueConsumerChannel;

  /**
   * Producer channels for sending return value messages to all other instances
   */
  std::map<HiCR::L0::Instance::instanceId_t, std::shared_ptr<HiCR::channel::variableSize::MPSC::locking::Producer>> _returnValueProducerChannels;

  /**
   * Consumer channels for receiving return value messages from all other instances
   */
  std::shared_ptr<HiCR::channel::fixedSize::MPSC::nonlocking::Consumer> _RPCConsumerChannel;

  /**
   * Producer channels for sending return value messages to all other instances
   */
  std::map<HiCR::L0::Instance::instanceId_t, std::shared_ptr<HiCR::channel::fixedSize::MPSC::nonlocking::Producer>> _RPCProducerChannels;

  /**
   * Map of execute units, representing potential RPC requests
   */
  std::map<RPCTargetIndex_t, std::shared_ptr<HiCR::L0::ExecutionUnit>> _RPCTargetMap;
};

} // namespace HiCR::frontend
