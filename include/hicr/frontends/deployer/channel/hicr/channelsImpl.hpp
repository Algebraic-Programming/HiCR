#define _HICR_DEPLOYER_CHANNEL_PAYLOAD_CAPACITY 1048576
#define _HICR_DEPLOYER_CHANNEL_COUNT_CAPACITY 1024
#define _HICR_DEPLOYER_CHANNEL_BASE_TAG 0xF0000000
#define _HICR_DEPLOYER_CHANNEL_CONSUMER_SIZES_BUFFER_TAG _HICR_DEPLOYER_CHANNEL_BASE_TAG + 0
#define _HICR_DEPLOYER_CHANNEL_CONSUMER_PAYLOAD_BUFFER_TAG _HICR_DEPLOYER_CHANNEL_BASE_TAG + 1
#define _HICR_DEPLOYER_CHANNEL_CONSUMER_COORDINATION_BUFFER_SIZES_TAG _HICR_DEPLOYER_CHANNEL_BASE_TAG + 3
#define _HICR_DEPLOYER_CHANNEL_CONSUMER_COORDINATION_BUFFER_PAYLOADS_TAG _HICR_DEPLOYER_CHANNEL_BASE_TAG + 4
#define _HICR_DEPLOYER_CHANNEL_PRODUCER_COORDINATION_BUFFER_SIZES_TAG _HICR_DEPLOYER_CHANNEL_BASE_TAG + 5
#define _HICR_DEPLOYER_CHANNEL_PRODUCER_COORDINATION_BUFFER_PAYLOADS_TAG _HICR_DEPLOYER_CHANNEL_BASE_TAG + 6

__INLINE__ void Instance::initializeChannels()
{
  // Getting my current instance
  const auto currentInstanceId = _instanceManager->getCurrentInstance()->getId();

  ////////// Creating consumer channel to receive variable sized messages from other instances

  // Accessing first topology manager detected
  auto &tm = *_topologyManagers[0];

  // Gathering topology from the topology manager
  const auto t = tm.queryTopology();

  // Selecting first device
  auto d = *t.getDevices().begin();

  // Getting memory space list from device
  auto memSpaces = d->getMemorySpaceList();

  // Grabbing first memory space for buffering
  auto bufferMemorySpace = *memSpaces.begin();

  // Getting required buffer sizes
  auto tokenSizeBufferSize = HiCR::channel::variableSize::Base::getTokenBufferSize(sizeof(size_t), _HICR_DEPLOYER_CHANNEL_COUNT_CAPACITY);

  // Allocating token size buffer as a local memory slot
  auto tokenSizeBufferSlot = _memoryManager->allocateLocalMemorySlot(bufferMemorySpace, tokenSizeBufferSize);

  // Allocating token size buffer as a local memory slot
  auto payloadBufferSlot = _memoryManager->allocateLocalMemorySlot(bufferMemorySpace, _HICR_DEPLOYER_CHANNEL_PAYLOAD_CAPACITY);

  // Getting required buffer size for coordination buffers
  auto coordinationBufferSize = HiCR::channel::variableSize::Base::getCoordinationBufferSize();

  // Allocating coordination buffers
  auto localConsumerCoordinationBufferMessageSizes    = _memoryManager->allocateLocalMemorySlot(bufferMemorySpace, coordinationBufferSize);
  auto localConsumerCoordinationBufferMessagePayloads = _memoryManager->allocateLocalMemorySlot(bufferMemorySpace, coordinationBufferSize);

  // Initializing coordination buffers
  HiCR::channel::variableSize::Base::initializeCoordinationBuffer(localConsumerCoordinationBufferMessageSizes);
  HiCR::channel::variableSize::Base::initializeCoordinationBuffer(localConsumerCoordinationBufferMessagePayloads);

  // Exchanging local memory slots to become global for them to be used by the remote end
  _communicationManager->exchangeGlobalMemorySlots(_HICR_DEPLOYER_CHANNEL_CONSUMER_SIZES_BUFFER_TAG, {{currentInstanceId, tokenSizeBufferSlot}});
  _communicationManager->fence(_HICR_DEPLOYER_CHANNEL_CONSUMER_SIZES_BUFFER_TAG);

  _communicationManager->exchangeGlobalMemorySlots(_HICR_DEPLOYER_CHANNEL_CONSUMER_PAYLOAD_BUFFER_TAG, {{currentInstanceId, payloadBufferSlot}});
  _communicationManager->fence(_HICR_DEPLOYER_CHANNEL_CONSUMER_PAYLOAD_BUFFER_TAG);

  _communicationManager->exchangeGlobalMemorySlots(_HICR_DEPLOYER_CHANNEL_CONSUMER_COORDINATION_BUFFER_SIZES_TAG,
                                                   {{currentInstanceId, localConsumerCoordinationBufferMessageSizes}});
  _communicationManager->fence(_HICR_DEPLOYER_CHANNEL_CONSUMER_COORDINATION_BUFFER_SIZES_TAG);

  _communicationManager->exchangeGlobalMemorySlots(_HICR_DEPLOYER_CHANNEL_CONSUMER_COORDINATION_BUFFER_PAYLOADS_TAG,
                                                   {{currentInstanceId, localConsumerCoordinationBufferMessagePayloads}});
  _communicationManager->fence(_HICR_DEPLOYER_CHANNEL_CONSUMER_COORDINATION_BUFFER_PAYLOADS_TAG);

  // Obtaining the globally exchanged memory slots
  auto consumerMessagePayloadBuffer      = _communicationManager->getGlobalMemorySlot(_HICR_DEPLOYER_CHANNEL_CONSUMER_PAYLOAD_BUFFER_TAG, currentInstanceId);
  auto consumerMessageSizesBuffer        = _communicationManager->getGlobalMemorySlot(_HICR_DEPLOYER_CHANNEL_CONSUMER_SIZES_BUFFER_TAG, currentInstanceId);
  auto consumerCoodinationPayloadsBuffer = _communicationManager->getGlobalMemorySlot(_HICR_DEPLOYER_CHANNEL_CONSUMER_COORDINATION_BUFFER_PAYLOADS_TAG, currentInstanceId);
  auto consumerCoordinationSizesBuffer   = _communicationManager->getGlobalMemorySlot(_HICR_DEPLOYER_CHANNEL_CONSUMER_COORDINATION_BUFFER_SIZES_TAG, currentInstanceId);

  // Creating channel
  _consumerChannel = std::make_shared<HiCR::channel::variableSize::MPSC::locking::Consumer>(*_communicationManager,
                                                                                            consumerMessagePayloadBuffer,
                                                                                            consumerMessageSizesBuffer,
                                                                                            localConsumerCoordinationBufferMessageSizes,
                                                                                            localConsumerCoordinationBufferMessagePayloads,
                                                                                            consumerCoordinationSizesBuffer,
                                                                                            consumerCoodinationPayloadsBuffer,
                                                                                            _HICR_DEPLOYER_CHANNEL_PAYLOAD_CAPACITY,
                                                                                            sizeof(uint8_t),
                                                                                            _HICR_DEPLOYER_CHANNEL_COUNT_CAPACITY);

  // Creating producer channels
  for (const auto &instance : _instanceManager->getInstances())
  {
    // Getting consumer instance id
    const auto consumerInstanceId = instance->getId();

    // Allocating coordination buffers
    auto localProducerSizeInfoBuffer                    = _memoryManager->allocateLocalMemorySlot(bufferMemorySpace, sizeof(size_t));
    auto localProducerCoordinationBufferMessageSizes    = _memoryManager->allocateLocalMemorySlot(bufferMemorySpace, coordinationBufferSize);
    auto localProducerCoordinationBufferMessagePayloads = _memoryManager->allocateLocalMemorySlot(bufferMemorySpace, coordinationBufferSize);

    // Initializing coordination buffers
    HiCR::channel::variableSize::Base::initializeCoordinationBuffer(localProducerCoordinationBufferMessageSizes);
    HiCR::channel::variableSize::Base::initializeCoordinationBuffer(localProducerCoordinationBufferMessagePayloads);

    // Obtaining the globally exchanged memory slots
    auto consumerMessagePayloadBuffer  = _communicationManager->getGlobalMemorySlot(_HICR_DEPLOYER_CHANNEL_CONSUMER_PAYLOAD_BUFFER_TAG, consumerInstanceId);
    auto consumerMessageSizesBuffer    = _communicationManager->getGlobalMemorySlot(_HICR_DEPLOYER_CHANNEL_CONSUMER_SIZES_BUFFER_TAG, consumerInstanceId);
    auto consumerPayloadConsumerBuffer = _communicationManager->getGlobalMemorySlot(_HICR_DEPLOYER_CHANNEL_CONSUMER_COORDINATION_BUFFER_PAYLOADS_TAG, consumerInstanceId);
    auto consumerSizesConsumerBuffer   = _communicationManager->getGlobalMemorySlot(_HICR_DEPLOYER_CHANNEL_CONSUMER_COORDINATION_BUFFER_SIZES_TAG, consumerInstanceId);

    // Creating channel
    _producerChannels[consumerInstanceId] = std::make_shared<HiCR::channel::variableSize::MPSC::locking::Producer>(*_communicationManager,
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

__INLINE__ void Instance::finalizeChannels() {}

__INLINE__ void Instance::sendMessage(const HiCR::L0::Instance::instanceId_t instanceId, void *messagePtr, size_t messageSize)
{
  // Sanity check
  if (_producerChannels.contains(instanceId) == false) HICR_THROW_RUNTIME("Instance Id %lu not found in the producer channel map");

  // Getting reference to the appropriate producer channel
  const auto &channel = _producerChannels[instanceId];

  // Accessing first topology manager detected
  auto &tm = *_topologyManagers[0];

  // Gathering topology from the topology manager
  const auto t = tm.queryTopology();

  // Selecting first device
  auto d = *t.getDevices().begin();

  // Getting memory space list from device
  auto memSpaces = d->getMemorySpaceList();

  // Grabbing first memory space for buffering
  auto bufferMemorySpace = *memSpaces.begin();

  // Sending initial message to all workers
  auto messageSendSlot = _memoryManager->registerLocalMemorySlot(bufferMemorySpace, messagePtr, messageSize);

  // Single-threaded access to channel
  std::lock_guard guard(_channelMutex);

  // Pushing message to channel
  channel->push(messageSendSlot);
}

__INLINE__ Instance::message_t Instance::recvMessage(const bool isAsync)
{
  // Single-threaded access to channel
  std::lock_guard guard(_channelMutex);

  // If asynchronous and consumerChannel is empty, return immediately
  if (isAsync == true && _consumerChannel->getDepth() == 0) return {NULL, 0};

  // Waiting for initial message from coordinator
  while (_consumerChannel->getDepth() == 0)
    ;

  // Get internal pointer of the token buffer slot and the offset
  auto payloadBufferMemorySlot = _consumerChannel->getPayloadBufferMemorySlot();
  auto payloadBufferPtr        = (const char *)payloadBufferMemorySlot->getSourceLocalMemorySlot()->getPointer();

  // Obtaining pointer from the offset + base pointer
  auto        offset = _consumerChannel->peek()[0];
  const void *ptr    = &payloadBufferPtr[offset];

  // Obtaining size
  const size_t size = _consumerChannel->peek()[1];

  // Popping message from the _consumerChannel
  _consumerChannel->pop();

  // Returning ptr + size pair
  return message_t{.data = ptr, .size = size};
}
