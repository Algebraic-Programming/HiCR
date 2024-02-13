/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file worker.hpp
 * @brief Provides a definition for the Runtime Coordinator class.
 * @author S. M. Martin
 * @date 5/2/2024
 */

#pragma once

#include <memory>
#include <hicr/definitions.hpp>
#include <hicr/exceptions.hpp>
#include "instance.hpp"

// For interoperability with YuanRong, we bifurcate implementations using different includes
#ifdef _HICR_USE_YUANRONG_BACKEND_
  #include <channel/yuanrong/producerChannel.hpp>
#else
  #include "channel/hicr/producerChannel.hpp"
#endif

namespace HiCR
{

namespace runtime
{

/**
 * Defines the coordinator class, which represents a self-contained instance of HiCR with access to compute and memory resources.
 *
 * There can be only a single coordinator per deployment and its job is to supervise the worker instances
 */
class Coordinator final : public runtime::Instance
{
  public:

  /**
   * Storage for worker data as required by the coordinator for issuing RPCs and sending messages
   */
  struct worker_t
  {
    /**
     * Initial entry point function name being executed by the worker
     */
    const std::string entryPoint;

    /**
     * Internal HiCR instance class
     */
    const std::shared_ptr<HiCR::L0::Instance> hicrInstance;

    /**
     * Producer channels to send messages to the worker instances
     */
    std::shared_ptr<runtime::ProducerChannel> channel;
  };

  /**
   * Constructor for the coordinator class
   *
   * @param[in] instanceManager The instance manager backend to use
   * @param[in] communicationManager The communication manager backend to use
   * @param[in] memoryManager The memory manager backend to use
   * @param[in] topologyManagers The topology managers backend to use to discover the system's resources
   * @param[in] machineModel The machine model to use to deploy the workers
   */
  Coordinator(HiCR::L1::InstanceManager &instanceManager,
              HiCR::L1::CommunicationManager &communicationManager,
              HiCR::L1::MemoryManager &memoryManager,
              std::vector<HiCR::L1::TopologyManager *> &topologyManagers,
              HiCR::MachineModel &machineModel) : runtime::Instance(instanceManager, communicationManager, memoryManager, topologyManagers, machineModel) {}

  Coordinator() = delete;
  ~Coordinator() = default;

  __USED__ inline void initialize() override {}

  /**
   * Deploys the requested machine model and uses the a user-provided acceptance criteria function to evaluate whether the allotted resources satisfy the request
   *
   * @param[in] requests A vector of instance requests, expressing the requested system's machine model and the tasks that each instance needs to run
   * @param[in] acceptanceCriteriaFc A user-given function that compares the requested topology for a given instance and the one obtained to decide whether it meets the user requirements
   * @param[in] argc The number of command line arguments
   * @param[in] argv Poiners to the character string for the command line arguments
   */
  __USED__ inline void deploy(std::vector<HiCR::MachineModel::request_t> &requests, HiCR::MachineModel::topologyAcceptanceCriteriaFc_t acceptanceCriteriaFc, int argc, char *argv[])
  {
    // Execute requests by finding or creating an instance that matches their topology requirements
    try
    {
      _machineModel->deploy(requests, acceptanceCriteriaFc, argc, argv);
    }
    catch (const std::exception &e)
    {
      fprintf(stderr, "Error while executing requests. Reason: '%s'\n", e.what());
      _abort(-1);
    }

    // Running the assigned task id in the corresponding instance and registering it as worker
    for (auto &r : requests)
      for (auto &in : r.instances)
      {
        // Creating worker instance
        auto worker = worker_t{.entryPoint = r.entryPointName, .hicrInstance = in};

        // Adding worker to the set
        _workers.push_back(worker);
      }

    // Launching channel creation routine for every worker
    for (auto &w : _workers) _instanceManager->launchRPC(*w.hicrInstance, "__initializeChannels");

    // Initializing RPC channels
    initializeChannels();

    // Launching worker's entry point function
    for (auto &w : _workers) _instanceManager->launchRPC(*w.hicrInstance, w.entryPoint);
  }

  __USED__ inline void finalize()
  {
    // Launching finalization RPC
    for (auto &w : _workers) _instanceManager->launchRPC(*w.hicrInstance, "__finalize");

    // Waiting for return ack
    for (auto &w : _workers) _instanceManager->getReturnValue(*w.hicrInstance);

    _instanceManager->finalize();
  }

  /**
   * Asynchronously sends a binary message (buffer + size) to a given worker
   *
   * @param[in] worker The recepient worker of the message
   * @param[in] messagePtr The pointer to the message buffer
   * @param[in] messageSize The message size in bytes
   */
  __USED__ inline void sendMessage(worker_t &worker, void *messagePtr, size_t messageSize);

  /**
   * Gets the worker vector, as deployed by the coordinator
   *
   * @return A reference to the worker vector
   */
  __USED__ inline std::vector<worker_t> &getWorkers() { return _workers; }

  /**
   * Requests the creation of a new data object.
   * 
   * This adds the internal counter for the assignment of unique identifiers to new data objects
   * 
   * @param[in] buffer A pointer to the internal buffer to assign to the data object
   * @param[in] size The size of the internal buffer to use
   * @return A shared pointer to the newly created data object
  */
  __USED__ inline std::shared_ptr<DataObject> createDataObject(void* buffer, const size_t size) { return std::make_shared<DataObject>(buffer, size, _currentDataObjectId++); }

  private:

  // For interoperability with YuanRong, this function is implemented differently depended on the backend used
  __USED__ inline void initializeChannels();

  /**
   * Storage for the deployed workers. This object is only mantained and usable by the coordinator
   */
  std::vector<worker_t> _workers;
};

// For interoperability with YuanRong, we bifurcate implementations using different includes
#ifdef _HICR_USE_YUANRONG_BACKEND_
  #include <channel/yuanrong/producerChannelImpl.hpp>
#else
  #include "channel/hicr/producerChannelImpl.hpp"
#endif

} // namespace runtime

} // namespace HiCR
