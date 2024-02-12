__USED__ inline void Worker::initializeChannels()
{
    printf("[Worker] Initializing Worker Instance...\n"); fflush(stdout);

    ////////// Creating consumer channel to receive variable sized RPCs from the coordinator

    // Accessing first topology manager detected
    auto& tm = *_topologyManagers[0];

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
    auto coordinationBufferMessageSizes    = _memoryManager->allocateLocalMemorySlot(bufferMemorySpace, coordinationBufferSize);
    auto coordinationBufferMessagePayloads = _memoryManager->allocateLocalMemorySlot(bufferMemorySpace, coordinationBufferSize);

    // Initializing coordination buffers
    HiCR::channel::variableSize::Base::initializeCoordinationBuffer(coordinationBufferMessageSizes);
    HiCR::channel::variableSize::Base::initializeCoordinationBuffer(coordinationBufferMessagePayloads);

    // Getting instance id for memory slot exchange
    const auto instanceId = _instanceManager->getCurrentInstance()->getId(); 
        
    // Exchanging local memory slots to become global for them to be used by the remote end
    _communicationManager->exchangeGlobalMemorySlots(_HICR_RUNTIME_CHANNEL_WORKER_SIZES_BUFFER_TAG,                      {{instanceId, tokenSizeBufferSlot}});
    _communicationManager->fence(_HICR_RUNTIME_CHANNEL_WORKER_SIZES_BUFFER_TAG);

    _communicationManager->exchangeGlobalMemorySlots(_HICR_RUNTIME_CHANNEL_WORKER_PAYLOAD_BUFFER_TAG,                    {{instanceId, payloadBufferSlot}});
    _communicationManager->fence(_HICR_RUNTIME_CHANNEL_WORKER_PAYLOAD_BUFFER_TAG);

    _communicationManager->exchangeGlobalMemorySlots(_HICR_RUNTIME_CHANNEL_WORKER_COORDINATION_BUFFER_SIZES_TAG,         {{instanceId, coordinationBufferMessageSizes}});
    _communicationManager->fence(_HICR_RUNTIME_CHANNEL_WORKER_COORDINATION_BUFFER_SIZES_TAG);

    _communicationManager->exchangeGlobalMemorySlots(_HICR_RUNTIME_CHANNEL_WORKER_COORDINATION_BUFFER_PAYLOADS_TAG,      {{instanceId, coordinationBufferMessagePayloads}});
    _communicationManager->fence(_HICR_RUNTIME_CHANNEL_WORKER_COORDINATION_BUFFER_PAYLOADS_TAG);

    _communicationManager->exchangeGlobalMemorySlots(_HICR_RUNTIME_CHANNEL_COORDINATOR_COORDINATION_BUFFER_SIZES_TAG,    {});
    _communicationManager->fence(_HICR_RUNTIME_CHANNEL_COORDINATOR_COORDINATION_BUFFER_SIZES_TAG);

    _communicationManager->exchangeGlobalMemorySlots(_HICR_RUNTIME_CHANNEL_COORDINATOR_COORDINATION_BUFFER_PAYLOADS_TAG, {});
    _communicationManager->fence(_HICR_RUNTIME_CHANNEL_COORDINATOR_COORDINATION_BUFFER_PAYLOADS_TAG); 

    // Obtaining the globally exchanged memory slots
    auto workerMessageSizesBuffer            = _communicationManager->getGlobalMemorySlot(_HICR_RUNTIME_CHANNEL_WORKER_SIZES_BUFFER_TAG,                      instanceId);
    auto workerMessagePayloadBuffer          = _communicationManager->getGlobalMemorySlot(_HICR_RUNTIME_CHANNEL_WORKER_PAYLOAD_BUFFER_TAG,                    instanceId);
    auto coordinatorSizesCoordinatorBuffer   = _communicationManager->getGlobalMemorySlot(_HICR_RUNTIME_CHANNEL_COORDINATOR_COORDINATION_BUFFER_SIZES_TAG,    instanceId);
    auto coordinatorPayloadCoordinatorBuffer = _communicationManager->getGlobalMemorySlot(_HICR_RUNTIME_CHANNEL_COORDINATOR_COORDINATION_BUFFER_PAYLOADS_TAG, instanceId);

    // Creating channel
    channel = std::make_shared<HiCR::channel::variableSize::SPSC::Consumer>(
        *_communicationManager,
        workerMessagePayloadBuffer,
        workerMessageSizesBuffer,
        coordinationBufferMessageSizes,
        coordinationBufferMessagePayloads,
        coordinatorSizesCoordinatorBuffer,
        coordinatorPayloadCoordinatorBuffer,
        _HICR_RUNTIME_CHANNEL_PAYLOAD_CAPACITY,
        sizeof(uint8_t),
        _HICR_RUNTIME_CHANNEL_COUNT_CAPACITY
    );
}

__USED__ inline std::pair<const void*, size_t> Worker::recvMessage()
{
    // Waiting for initial message from coordinator
    while (channel->getDepth() == 0) channel->updateDepth();

    // Get internal pointer of the token buffer slot and the offset
    auto payloadBufferMemorySlot = channel->getPayloadBufferMemorySlot();
    auto payloadBufferPtr = (const char*) payloadBufferMemorySlot->getSourceLocalMemorySlot()->getPointer();
    
    // Obtaining pointer from the offset + base pointer
    auto offset = channel->peek()[0];
    const void* ptr = &payloadBufferPtr[offset]; 

    // Obtaining size
    const size_t size = channel->peek()[1];

    // Popping message from the channel
    channel->pop();

    // Returning ptr + size pair
    return std::make_pair(ptr, size);
}