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
  #include <frontends/runtime/channel/yuanrong/consumerChannel.hpp>
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

  /**
   * Constructor for the coordinator class
   *
   * @param[in] instanceManager The instance manager backend to use
   * @param[in] communicationManager The communication manager backend to use
   * @param[in] memoryManager The memory manager backend to use
   * @param[in] topologyManagers The topology managers backend to use to discover the system's resources
   * @param[in] machineModel The machine model to use to deploy the workers
   */
  Worker(HiCR::L1::InstanceManager &instanceManager,
         HiCR::L1::CommunicationManager &communicationManager,
         HiCR::L1::MemoryManager &memoryManager,
         std::vector<HiCR::L1::TopologyManager *> &topologyManagers,
         HiCR::MachineModel &machineModel) : runtime::Instance(instanceManager, communicationManager, memoryManager, topologyManagers, machineModel) {}
  Worker() = delete;
  ~Worker() = default;

  __USED__ inline void initialize() override
  {
    // Starting to listen

    // Flag to indicate whether the worker should continue listening
    bool continueListening = true;

    // Adding RPC targets, specifying a name and the execution unit to execute
    _instanceManager->addRPCTarget("__finalize", [&]()
                                   { continueListening = false; });
    _instanceManager->addRPCTarget("__initializeChannels", [this]()
                                   { initializeChannels(); });

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

  /**
   * Synchronous function to receive a message from the coordinator instance
   *
   * @return A pair containing a pointer to the start of the message binary data and the message's size
   */
  __USED__ inline std::pair<const void *, size_t> recvMessage();

  /**
   * Allows a worker to obtain a data object by id from the coordinator instance
   *
   * This is a blocking function.
   * The data object must be published (either before or after this call) by the coordinator for this function to succeed.
   *
   * @param[in] dataObjectId The id of the data object to get from the coordinator instance
   * @return A shared pointer to the obtained data object
   */
  __USED__ inline std::shared_ptr<DataObject> getDataObject(const DataObject::dataObjectId_t dataObjectId)
  {
    // Getting instance id of coordinator instance
    const auto coordinatorId = _instanceManager->getRootInstanceId();
    const auto currentInstanceId = _instanceManager->getCurrentInstance()->getId();
    // Creating data object from id and remote instance id
    return DataObject::getDataObject(dataObjectId, coordinatorId, currentInstanceId);
  }

  private:

  /**
   * Initializes the consumer channel for the worker to receive messages from the coordinator
   */
  __USED__ inline void initializeChannels();

  /**
   * Consumer channel to receive messages from the coordinator instance
   */
  std::shared_ptr<runtime::ConsumerChannel> channel;
};

// For interoperability with YuanRong, we bifurcate implementations using different includes
#ifdef _HICR_USE_YUANRONG_BACKEND_
  #include <frontends/runtime/channel/yuanrong/consumerChannelImpl.hpp>
#else
  #include "channel/hicr/consumerChannelImpl.hpp"
#endif

} // namespace runtime

} // namespace HiCR
