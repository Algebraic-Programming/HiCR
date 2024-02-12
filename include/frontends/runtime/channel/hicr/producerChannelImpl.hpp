#pragma once

__USED__ inline void Coordinator::initializeChannels()
{
////////// Creating producer channels to send variable sized RPCs fto the workers

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

// Getting required buffer size for coordination buffers
auto coordinationBufferSize = HiCR::channel::variableSize::Base::getCoordinationBufferSize();

// Storage for dedicated channels
std::vector<std::pair<HiCR::L0::Instance::instanceId_t, std::shared_ptr<HiCR::L0::LocalMemorySlot>>> coordinationBufferMessageSizesVector;
std::vector<std::pair<HiCR::L0::Instance::instanceId_t, std::shared_ptr<HiCR::L0::LocalMemorySlot>>> coordinationBufferMessagePayloadsVector;
std::vector<std::shared_ptr<HiCR::L0::LocalMemorySlot>> sizeInfoBufferMemorySlotVector;

// Create coordinator buffers for each of the channels (one per worker)
for (size_t i = 0; i < _workers.size(); i++)
{
    // Allocating coordination buffers
    auto coordinationBufferMessageSizes    = _memoryManager->allocateLocalMemorySlot(bufferMemorySpace, coordinationBufferSize);
    auto coordinationBufferMessagePayloads = _memoryManager->allocateLocalMemorySlot(bufferMemorySpace, coordinationBufferSize);
    auto sizeInfoBufferMemorySlot          = _memoryManager->allocateLocalMemorySlot(bufferMemorySpace, sizeof(size_t));

    // Initializing coordination buffers
    HiCR::channel::variableSize::Base::initializeCoordinationBuffer(coordinationBufferMessageSizes);
    HiCR::channel::variableSize::Base::initializeCoordinationBuffer(coordinationBufferMessagePayloads);

    // Getting worker instance id
    const auto workerInstanceId = _workers[i].hicrInstance->getId();

    // Adding buffers to their respective vectors
    coordinationBufferMessageSizesVector.push_back({workerInstanceId, coordinationBufferMessageSizes});
    coordinationBufferMessagePayloadsVector.push_back({workerInstanceId, coordinationBufferMessagePayloads});
    sizeInfoBufferMemorySlotVector.push_back(sizeInfoBufferMemorySlot);
}

// Exchanging local memory slots to become global for them to be used by the remote end
_communicationManager->exchangeGlobalMemorySlots(_HICR_RUNTIME_CHANNEL_WORKER_SIZES_BUFFER_TAG,                      {});
_communicationManager->fence(_HICR_RUNTIME_CHANNEL_WORKER_SIZES_BUFFER_TAG);

_communicationManager->exchangeGlobalMemorySlots(_HICR_RUNTIME_CHANNEL_WORKER_PAYLOAD_BUFFER_TAG,                    {});
_communicationManager->fence(_HICR_RUNTIME_CHANNEL_WORKER_PAYLOAD_BUFFER_TAG);

_communicationManager->exchangeGlobalMemorySlots(_HICR_RUNTIME_CHANNEL_WORKER_COORDINATION_BUFFER_SIZES_TAG,         {});
_communicationManager->fence(_HICR_RUNTIME_CHANNEL_WORKER_COORDINATION_BUFFER_SIZES_TAG);

_communicationManager->exchangeGlobalMemorySlots(_HICR_RUNTIME_CHANNEL_WORKER_COORDINATION_BUFFER_PAYLOADS_TAG,      {});
_communicationManager->fence(_HICR_RUNTIME_CHANNEL_WORKER_COORDINATION_BUFFER_PAYLOADS_TAG);

_communicationManager->exchangeGlobalMemorySlots(_HICR_RUNTIME_CHANNEL_COORDINATOR_COORDINATION_BUFFER_SIZES_TAG,    coordinationBufferMessageSizesVector);
_communicationManager->fence(_HICR_RUNTIME_CHANNEL_COORDINATOR_COORDINATION_BUFFER_SIZES_TAG);

_communicationManager->exchangeGlobalMemorySlots(_HICR_RUNTIME_CHANNEL_COORDINATOR_COORDINATION_BUFFER_PAYLOADS_TAG, coordinationBufferMessagePayloadsVector);
_communicationManager->fence(_HICR_RUNTIME_CHANNEL_COORDINATOR_COORDINATION_BUFFER_PAYLOADS_TAG); 

// Creating producer channels
for (size_t i = 0; i < _workers.size(); i++)
{
    // Getting worker
    auto& worker = _workers[i];

    // Getting worker instance id
    const auto workerInstanceId = worker.hicrInstance->getId();

    // Obtaining the globally exchanged memory slots
    auto workerMessageSizesBuffer            = _communicationManager->getGlobalMemorySlot(_HICR_RUNTIME_CHANNEL_WORKER_SIZES_BUFFER_TAG,                      workerInstanceId);
    auto workerMessagePayloadBuffer          = _communicationManager->getGlobalMemorySlot(_HICR_RUNTIME_CHANNEL_WORKER_PAYLOAD_BUFFER_TAG,                    workerInstanceId);
    auto coordinatorSizesCoordinatorBuffer   = _communicationManager->getGlobalMemorySlot(_HICR_RUNTIME_CHANNEL_COORDINATOR_COORDINATION_BUFFER_SIZES_TAG,    workerInstanceId);
    auto coordinatorPayloadCoordinatorBuffer = _communicationManager->getGlobalMemorySlot(_HICR_RUNTIME_CHANNEL_COORDINATOR_COORDINATION_BUFFER_PAYLOADS_TAG, workerInstanceId);

    // Creating channel
    worker.channel = std::make_shared<HiCR::channel::variableSize::SPSC::Producer>(
    *_communicationManager,
    sizeInfoBufferMemorySlotVector[i],
    workerMessagePayloadBuffer,
    workerMessageSizesBuffer,
    coordinationBufferMessageSizesVector[i].second,
    coordinationBufferMessagePayloadsVector[i].second,
    coordinatorSizesCoordinatorBuffer,
    coordinatorPayloadCoordinatorBuffer,
    _HICR_RUNTIME_CHANNEL_PAYLOAD_CAPACITY,
    sizeof(uint8_t),
    _HICR_RUNTIME_CHANNEL_COUNT_CAPACITY
    );
 }
}

__USED__ inline void Coordinator::sendMessage(worker_t& worker, void* messagePtr, size_t messageSize)
{
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

    // Sending initial message to all workers
    auto messageSendSlot = _memoryManager->registerLocalMemorySlot(bufferMemorySpace, messagePtr, messageSize);
    worker.channel->push(messageSendSlot);
}