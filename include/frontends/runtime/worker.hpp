/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file worker.hpp
 * @brief Provides a definition for the Runtime worker class.
 * @author S. M. Martin
 * @date 5/2/2024
 */

#pragma once

#include <memory>
#include <hicr/definitions.hpp>
#include "channel.hpp"
#include "instance.hpp"

namespace HiCR
{

namespace runtime
{

/**
 * Defines the worker class, which represents a self-contained instance of HiCR with access to compute and memory resources.
 *
 * Workers may be created during runtime (if the process managing backend allows for it), or activated/suspended on demand.
 */
class Worker final : public runtime::Instance
{
  public:


  Worker() = delete;
  Worker( HiCR::L1::InstanceManager& instanceManager,
          HiCR::L1::CommunicationManager& communicationManager,
          HiCR::L1::MemoryManager& memoryManager,
          std::vector<HiCR::L1::TopologyManager*>& topologyManagers,
          HiCR::MachineModel& machineModel
        ) : runtime::Instance(instanceManager, communicationManager, memoryManager, topologyManagers, machineModel) {}
  ~Worker() = default;

  __USED__ inline void initialize() override
  {
    // Starting to listen 

    // Flag to indicate whether the worker should continue listening
    bool continueListening = true;

    // Adding RPC targets, specifying a name and the execution unit to execute
    _instanceManager->addRPCTarget("__finalize", [&]() { continueListening = false; });
    _instanceManager->addRPCTarget("__initializeRPCChannels", [this]() { this->initializeRPCChannels(); });

    // Listening for RPC requests
    while (continueListening == true) _instanceManager->listen();

    // Finalizing
    finalize();
  }

  __USED__ inline void finalize() override 
  {
    uint8_t ack = 0;

    // Registering an empty return value to ack on finalization
     _instanceManager->submitReturnValue(&ack, sizeof(ack));

    // Finalizing instance manager
    _instanceManager->finalize();

    // Exiting now
    exit(0);
  }

  private:

  __USED__ inline void initializeRPCChannels()
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
    auto consumerChannel = HiCR::channel::variableSize::SPSC::Consumer(
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

    // Waiting for initial message from coordinator
    while (consumerChannel.getDepth() == 0) consumerChannel.updateDepth();

    // Get internal pointer of the token buffer slot and the offset
    auto payloadBufferPtr = (const char*) payloadBufferSlot->getPointer();
    auto offset = consumerChannel.peek()[0];
    consumerChannel.pop();

    // Printing message
    printf("[Worker] Message from the coordinator: '%s'\n", &payloadBufferPtr[offset]);
  }
};

} // namespace runtime

} // namespace HiCR
