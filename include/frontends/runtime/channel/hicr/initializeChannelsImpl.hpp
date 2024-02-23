#define _HICR_RUNTIME_CHANNEL_PAYLOAD_CAPACITY 1048576
#define _HICR_RUNTIME_CHANNEL_COUNT_CAPACITY 1024
#define _HICR_RUNTIME_CHANNEL_BASE_TAG 0xF0000000
#define _HICR_RUNTIME_CHANNEL_CONSUMER_SIZES_BUFFER_TAG _HICR_RUNTIME_CHANNEL_BASE_TAG + 0
#define _HICR_RUNTIME_CHANNEL_CONSUMER_PAYLOAD_BUFFER_TAG _HICR_RUNTIME_CHANNEL_BASE_TAG + 1
#define _HICR_RUNTIME_CHANNEL_CONSUMER_COORDINATION_BUFFER_SIZES_TAG _HICR_RUNTIME_CHANNEL_BASE_TAG + 3
#define _HICR_RUNTIME_CHANNEL_CONSUMER_COORDINATION_BUFFER_PAYLOADS_TAG _HICR_RUNTIME_CHANNEL_BASE_TAG + 4
#define _HICR_RUNTIME_CHANNEL_PRODUCER_COORDINATION_BUFFER_SIZES_TAG _HICR_RUNTIME_CHANNEL_BASE_TAG + 5
#define _HICR_RUNTIME_CHANNEL_PRODUCER_COORDINATION_BUFFER_PAYLOADS_TAG _HICR_RUNTIME_CHANNEL_BASE_TAG + 6

__USED__ inline void Instance::initializeChannels()
{
  // Getting my current instance
  const auto currentInstanceId = _instanceManager->getCurrentInstance()->getId();

  // Update the instance ids
  queryInstanceIds();

  // Getting total amount of instances
  const auto totalInstances = _instanceIds.size();

  // Create N-1 producer-consumer pairs for each instance
  for (size_t producerIdx = 0; producerIdx < totalInstances; producerIdx++)
  {
    // If the producer Id is this instance, then create all producer channels for the other instances
    if (_instanceIds[producerIdx] == currentInstanceId)
    {
      ////////// Creating producer channels to send variable sized RPCs fto the consumers

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

      // Getting required buffer size for coordination buffers
      auto coordinationBufferSize = HiCR::channel::variableSize::Base::getCoordinationBufferSize();

      // Storage for dedicated channels
      std::vector<std::pair<HiCR::L0::Instance::instanceId_t, std::shared_ptr<HiCR::L0::LocalMemorySlot>>> coordinationBufferMessageSizesVector;
      std::vector<std::pair<HiCR::L0::Instance::instanceId_t, std::shared_ptr<HiCR::L0::LocalMemorySlot>>> coordinationBufferMessagePayloadsVector;
      std::vector<std::shared_ptr<HiCR::L0::LocalMemorySlot>> sizeInfoBufferMemorySlotVector;

      // Create producer buffers for each of the channels (one per consumer)
      for (size_t i = 0; i < totalInstances; i++)
        if (i != producerIdx)
        {
          // Allocating coordination buffers
          auto coordinationBufferMessageSizes = _memoryManager->allocateLocalMemorySlot(bufferMemorySpace, coordinationBufferSize);
          auto coordinationBufferMessagePayloads = _memoryManager->allocateLocalMemorySlot(bufferMemorySpace, coordinationBufferSize);
          auto sizeInfoBufferMemorySlot = _memoryManager->allocateLocalMemorySlot(bufferMemorySpace, sizeof(size_t));

          // Initializing coordination buffers
          HiCR::channel::variableSize::Base::initializeCoordinationBuffer(coordinationBufferMessageSizes);
          HiCR::channel::variableSize::Base::initializeCoordinationBuffer(coordinationBufferMessagePayloads);

          // Getting consumer instance id
          const auto consumerInstanceId = _instanceIds[i];

          // Adding buffers to their respective vectors
          coordinationBufferMessageSizesVector.push_back({consumerInstanceId, coordinationBufferMessageSizes});
          coordinationBufferMessagePayloadsVector.push_back({consumerInstanceId, coordinationBufferMessagePayloads});
          sizeInfoBufferMemorySlotVector.push_back(sizeInfoBufferMemorySlot);
        }

      // Exchanging local memory slots to become global for them to be used by the remote end
      _communicationManager->exchangeGlobalMemorySlots(_HICR_RUNTIME_CHANNEL_CONSUMER_SIZES_BUFFER_TAG, {});
      _communicationManager->fence(_HICR_RUNTIME_CHANNEL_CONSUMER_SIZES_BUFFER_TAG);

      _communicationManager->exchangeGlobalMemorySlots(_HICR_RUNTIME_CHANNEL_CONSUMER_PAYLOAD_BUFFER_TAG, {});
      _communicationManager->fence(_HICR_RUNTIME_CHANNEL_CONSUMER_PAYLOAD_BUFFER_TAG);

      _communicationManager->exchangeGlobalMemorySlots(_HICR_RUNTIME_CHANNEL_CONSUMER_COORDINATION_BUFFER_SIZES_TAG, {});
      _communicationManager->fence(_HICR_RUNTIME_CHANNEL_CONSUMER_COORDINATION_BUFFER_SIZES_TAG);

      _communicationManager->exchangeGlobalMemorySlots(_HICR_RUNTIME_CHANNEL_CONSUMER_COORDINATION_BUFFER_PAYLOADS_TAG, {});
      _communicationManager->fence(_HICR_RUNTIME_CHANNEL_CONSUMER_COORDINATION_BUFFER_PAYLOADS_TAG);

      _communicationManager->exchangeGlobalMemorySlots(_HICR_RUNTIME_CHANNEL_PRODUCER_COORDINATION_BUFFER_SIZES_TAG, coordinationBufferMessageSizesVector);
      _communicationManager->fence(_HICR_RUNTIME_CHANNEL_PRODUCER_COORDINATION_BUFFER_SIZES_TAG);

      _communicationManager->exchangeGlobalMemorySlots(_HICR_RUNTIME_CHANNEL_PRODUCER_COORDINATION_BUFFER_PAYLOADS_TAG, coordinationBufferMessagePayloadsVector);
      _communicationManager->fence(_HICR_RUNTIME_CHANNEL_PRODUCER_COORDINATION_BUFFER_PAYLOADS_TAG);

      // Creating producer channels
      size_t slotVectorIdx = 0;
      for (size_t i = 0; i < totalInstances; i++)
        if (i != producerIdx)
        {
          // Getting consumer instance id
          const auto consumerInstanceId = _instanceIds[i];

          // Obtaining the globally exchanged memory slots
          auto consumerMessageSizesBuffer = _communicationManager->getGlobalMemorySlot(_HICR_RUNTIME_CHANNEL_CONSUMER_SIZES_BUFFER_TAG, consumerInstanceId);
          auto consumerMessagePayloadBuffer = _communicationManager->getGlobalMemorySlot(_HICR_RUNTIME_CHANNEL_CONSUMER_PAYLOAD_BUFFER_TAG, consumerInstanceId);
          auto producerSizesProducerBuffer = _communicationManager->getGlobalMemorySlot(_HICR_RUNTIME_CHANNEL_PRODUCER_COORDINATION_BUFFER_SIZES_TAG, consumerInstanceId);
          auto producerPayloadProducerBuffer = _communicationManager->getGlobalMemorySlot(_HICR_RUNTIME_CHANNEL_PRODUCER_COORDINATION_BUFFER_PAYLOADS_TAG, consumerInstanceId);

          // Creating channel
          _producerChannels[consumerInstanceId] = std::make_shared<HiCR::channel::variableSize::SPSC::Producer>(
            *_communicationManager,
            sizeInfoBufferMemorySlotVector[slotVectorIdx],
            consumerMessagePayloadBuffer,
            consumerMessageSizesBuffer,
            coordinationBufferMessageSizesVector[slotVectorIdx].second,
            coordinationBufferMessagePayloadsVector[slotVectorIdx].second,
            producerSizesProducerBuffer,
            producerPayloadProducerBuffer,
            _HICR_RUNTIME_CHANNEL_PAYLOAD_CAPACITY,
            sizeof(uint8_t),
            _HICR_RUNTIME_CHANNEL_COUNT_CAPACITY);

          // Increasing slot vector
          slotVectorIdx++;
        }
    }

    // If the produer Id is not this instance, then create a consumer channel for the current producer
    if (_instanceIds[producerIdx] != currentInstanceId)
    {
      ////////// Creating consumer channel to receive variable sized RPCs from the producer

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
      auto tokenSizeBufferSize = HiCR::channel::variableSize::Base::getTokenBufferSize(sizeof(size_t), _HICR_RUNTIME_CHANNEL_COUNT_CAPACITY);

      // Allocating token size buffer as a local memory slot
      auto tokenSizeBufferSlot = _memoryManager->allocateLocalMemorySlot(bufferMemorySpace, tokenSizeBufferSize);

      // Allocating token size buffer as a local memory slot
      auto payloadBufferSlot = _memoryManager->allocateLocalMemorySlot(bufferMemorySpace, _HICR_RUNTIME_CHANNEL_PAYLOAD_CAPACITY);

      // Getting required buffer size for coordination buffers
      auto coordinationBufferSize = HiCR::channel::variableSize::Base::getCoordinationBufferSize();

      // Allocating coordination buffers
      auto coordinationBufferMessageSizes = _memoryManager->allocateLocalMemorySlot(bufferMemorySpace, coordinationBufferSize);
      auto coordinationBufferMessagePayloads = _memoryManager->allocateLocalMemorySlot(bufferMemorySpace, coordinationBufferSize);

      // Initializing coordination buffers
      HiCR::channel::variableSize::Base::initializeCoordinationBuffer(coordinationBufferMessageSizes);
      HiCR::channel::variableSize::Base::initializeCoordinationBuffer(coordinationBufferMessagePayloads);

      // Exchanging local memory slots to become global for them to be used by the remote end
      _communicationManager->exchangeGlobalMemorySlots(_HICR_RUNTIME_CHANNEL_CONSUMER_SIZES_BUFFER_TAG, {{currentInstanceId, tokenSizeBufferSlot}});
      _communicationManager->fence(_HICR_RUNTIME_CHANNEL_CONSUMER_SIZES_BUFFER_TAG);

      _communicationManager->exchangeGlobalMemorySlots(_HICR_RUNTIME_CHANNEL_CONSUMER_PAYLOAD_BUFFER_TAG, {{currentInstanceId, payloadBufferSlot}});
      _communicationManager->fence(_HICR_RUNTIME_CHANNEL_CONSUMER_PAYLOAD_BUFFER_TAG);

      _communicationManager->exchangeGlobalMemorySlots(_HICR_RUNTIME_CHANNEL_CONSUMER_COORDINATION_BUFFER_SIZES_TAG, {{currentInstanceId, coordinationBufferMessageSizes}});
      _communicationManager->fence(_HICR_RUNTIME_CHANNEL_CONSUMER_COORDINATION_BUFFER_SIZES_TAG);

      _communicationManager->exchangeGlobalMemorySlots(_HICR_RUNTIME_CHANNEL_CONSUMER_COORDINATION_BUFFER_PAYLOADS_TAG, {{currentInstanceId, coordinationBufferMessagePayloads}});
      _communicationManager->fence(_HICR_RUNTIME_CHANNEL_CONSUMER_COORDINATION_BUFFER_PAYLOADS_TAG);

      _communicationManager->exchangeGlobalMemorySlots(_HICR_RUNTIME_CHANNEL_PRODUCER_COORDINATION_BUFFER_SIZES_TAG, {});
      _communicationManager->fence(_HICR_RUNTIME_CHANNEL_PRODUCER_COORDINATION_BUFFER_SIZES_TAG);

      _communicationManager->exchangeGlobalMemorySlots(_HICR_RUNTIME_CHANNEL_PRODUCER_COORDINATION_BUFFER_PAYLOADS_TAG, {});
      _communicationManager->fence(_HICR_RUNTIME_CHANNEL_PRODUCER_COORDINATION_BUFFER_PAYLOADS_TAG);

      // Obtaining the globally exchanged memory slots
      auto consumerMessageSizesBuffer = _communicationManager->getGlobalMemorySlot(_HICR_RUNTIME_CHANNEL_CONSUMER_SIZES_BUFFER_TAG, currentInstanceId);
      auto consumerMessagePayloadBuffer = _communicationManager->getGlobalMemorySlot(_HICR_RUNTIME_CHANNEL_CONSUMER_PAYLOAD_BUFFER_TAG, currentInstanceId);
      auto producerSizesProducerBuffer = _communicationManager->getGlobalMemorySlot(_HICR_RUNTIME_CHANNEL_PRODUCER_COORDINATION_BUFFER_SIZES_TAG, currentInstanceId);
      auto producerPayloadProducerBuffer = _communicationManager->getGlobalMemorySlot(_HICR_RUNTIME_CHANNEL_PRODUCER_COORDINATION_BUFFER_PAYLOADS_TAG, currentInstanceId);

      // Creating channel
      printf("[Instance %lu] Adding consumer channel for instance %lu\n", currentInstanceId, _instanceIds[producerIdx]);
      _consumerChannels[_instanceIds[producerIdx]] = std::make_shared<HiCR::channel::variableSize::SPSC::Consumer>(
        *_communicationManager,
        consumerMessagePayloadBuffer,
        consumerMessageSizesBuffer,
        coordinationBufferMessageSizes,
        coordinationBufferMessagePayloads,
        producerSizesProducerBuffer,
        producerPayloadProducerBuffer,
        _HICR_RUNTIME_CHANNEL_PAYLOAD_CAPACITY,
        sizeof(uint8_t),
        _HICR_RUNTIME_CHANNEL_COUNT_CAPACITY);
    }
  }
}

__USED__ inline void Instance::sendMessage(const HiCR::L0::Instance::instanceId_t instanceId, void *messagePtr, size_t messageSize)
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
  channel->push(messageSendSlot);
}

__USED__ inline std::pair<const void *, size_t> Instance::recvMessage(const HiCR::L0::Instance::instanceId_t instanceId, const bool isAsync)
{
  if (_consumerChannels.contains(instanceId) == false) HICR_THROW_RUNTIME("Instance Id %lu not found in the consumer channel map");

  // Getting reference to the appropriate consumer channel
  const auto &channel = _consumerChannels[instanceId];

  // If asynchronous and channel is empty, return immediately
  if (isAsync == true && channel->getDepth() == 0) return {NULL, 0};

  // Waiting for initial message from coordinator
  while (channel->getDepth() == 0) channel->updateDepth();

  // Get internal pointer of the token buffer slot and the offset
  auto payloadBufferMemorySlot = channel->getPayloadBufferMemorySlot();
  auto payloadBufferPtr = (const char *)payloadBufferMemorySlot->getSourceLocalMemorySlot()->getPointer();

  // Obtaining pointer from the offset + base pointer
  auto offset = channel->peek()[0];
  const void *ptr = &payloadBufferPtr[offset];

  // Obtaining size
  const size_t size = channel->peek()[1];

  // Popping message from the channel
  channel->pop();

  // Returning ptr + size pair
  return std::make_pair(ptr, size);
}
