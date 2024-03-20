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

  /**
   * Constructor for the coordinator class
   *
   * @param[in] instanceManager The instance manager backend to use
   * @param[in] communicationManager The communication manager backend to use
   * @param[in] memoryManager The memory manager backend to use
   * @param[in] topologyManagers The topology managers backend to use to discover the system's resources
   * @param[in] machineModel The machine model to use to deploy the workers
   */
  Worker(HiCR::L1::InstanceManager*                instanceManager,
         HiCR::L1::CommunicationManager*           communicationManager,
         HiCR::L1::MemoryManager*                  memoryManager,
         std::vector<HiCR::L1::TopologyManager *>  topologyManagers,
         HiCR::MachineModel*                       machineModel)
    : runtime::Instance(instanceManager, communicationManager, memoryManager, topologyManagers, machineModel)
  {}
  Worker()  = delete;
  ~Worker() = default;

  __USED__ inline void initialize() override
  {
    // Starting to listen

    // Flag to indicate whether the worker should continue listening
    bool continueListening = true;

    // Adding RPC targets, specifying a name and the execution unit to execute
    _instanceManager->addRPCTarget("__finalize", [&]() { continueListening = false; });
    _instanceManager->addRPCTarget("__initializeChannels", [this]() { initializeChannels(); });

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
};

} // namespace runtime

} // namespace HiCR
