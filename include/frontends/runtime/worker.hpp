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
#include "instance.hpp"

// For interoperability with YuanRong, we bifurcate implementations using different includes
#ifdef _HICR_USE_YUANRONG_BACKEND_
  #include <channel/yuanrong/consumerChannel.hpp>
#else
  #include "channel/hicr/consumerChannel.hpp"
#endif

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
    _instanceManager->addRPCTarget("__initializeRPCChannels", [this]()
     {
        initializeRPCChannels();

        // Waiting for initial message from coordinator
        while (rpcChannel->getDepth() == 0) rpcChannel->updateDepth();

        // Get internal pointer of the token buffer slot and the offset
        auto payloadBufferMemorySlot = rpcChannel->getPayloadBufferMemorySlot();
        auto payloadBufferPtr = (const char*) payloadBufferMemorySlot->getSourceLocalMemorySlot()->getPointer();
        auto offset = rpcChannel->peek()[0];
        rpcChannel->pop();

        // Printing message
        printf("[Worker] Message from the coordinator: '%s'\n", &payloadBufferPtr[offset]);
     });

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

  __USED__ inline void initializeRPCChannels();

  /**
   * Consumer channel to receive RPCs from the coordinator instance
   */ 
  std::shared_ptr<runtime::ConsumerChannel> rpcChannel;
};

// For interoperability with YuanRong, we bifurcate implementations using different includes
#ifdef _HICR_USE_YUANRONG_BACKEND_
  #include "channel/yuanrong/consumerChannelImpl.hpp"
#else
  #include "channel/hicr/consumerChannelImpl.hpp"
#endif

} // namespace runtime

} // namespace HiCR
