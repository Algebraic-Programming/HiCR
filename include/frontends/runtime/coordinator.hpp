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
#include <hicr/L0/topology.hpp>
#include "instance.hpp"

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
     * The original request by which the worker was created
     */
    const HiCR::MachineModel::request_t request;

    /**
     * Internal HiCR instance class
     */
    const std::shared_ptr<HiCR::L0::Instance> hicrInstance;
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

    // Update instanceIds with the newly created instances
    queryInstanceIds();

    // Running the assigned task id in the corresponding instance and registering it as worker
    for (auto &r : requests)
      for (auto &in : r.instances)
      {
        // Creating worker instance
        auto worker = worker_t{.request = r, .hicrInstance = in};

        // Adding worker to the set
        _workers.push_back(worker);
      }

    // Launching channel creation routine for every worker
    for (auto &w : _workers) _instanceManager->launchRPC(*w.hicrInstance, "__initializeChannels");

    // Initializing RPC channels
    initializeChannels();

    // Launching worker's entry point function
    for (auto &w : _workers) _instanceManager->launchRPC(*w.hicrInstance, w.request.entryPointName);
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
   * Gets the worker vector, as deployed by the coordinator
   *
   * @return A reference to the worker vector
   */
  __USED__ inline std::vector<worker_t> &getWorkers() { return _workers; }

  private:

  /**
   * Storage for the deployed workers. This object is only mantained and usable by the coordinator
   */
  std::vector<worker_t> _workers;
};

} // namespace runtime

} // namespace HiCR
