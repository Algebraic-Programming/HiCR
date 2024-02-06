
/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file runtime.hpp
 * @brief This file implements the runtime class
 * @author S. M Martin
 * @date 31/01/2024
 */

#pragma once

#include <vector>
#include <hicr/L1/instanceManager.hpp>
#include <hicr/L1/memoryManager.hpp>
#include <hicr/L1/topologyManager.hpp>
#include <frontends/machineModel/machineModel.hpp>
#include "instance.hpp"

#ifdef _HICR_USE_MPI_BACKEND_
  #include <backends/mpi/L1/instanceManager.hpp>
#endif

#ifdef _HICR_USE_YUANRONG_BACKEND_
  #include <backends/yuanrong/L1/instanceManager.hpp>
#endif

#ifdef _HICR_USE_HWLOC_BACKEND_
  #include <backends/host/hwloc/L1/topologyManager.hpp>
#endif

#ifdef _HICR_USE_ASCEND_BACKEND_
  #include <backends/ascend/L1/topologyManager.hpp>
#endif

namespace HiCR
{

class Runtime;

/**
 * Static singleton of the HiCR runtime class
 */
static Runtime *_runtime;

/**
 * The runtime class represents a singleton that exposes many HiCR's (and its frontends') functionalities with a simplified API
 * and performs backend detect and initialization of a selected set of backends. This class goal is to be FF3 RTS's backend library
 * but it can used by other libraries as well.
 */
class Runtime final
{
  public:

  /**
   * The constructor for the HiCR runtime singleton takes the argc and argv pointers, as passed to the main() function
   *
   * @param[in] pargc Pointer to the argc argument count
   * @param[in] pargv Pointer to the argc argument char arrays
   */
  Runtime(int *pargc, char ***pargv) : _pargc(pargc), _pargv(pargv)
  {
/////////////////////////// Detecting instance manager

// Detecting MPI
#ifdef _HICR_USE_MPI_BACKEND_
    _instanceManager = HiCR::backend::mpi::L1::InstanceManager::createDefault(_pargc, _pargv);
#endif

// Detecting YuanRong
#ifdef _HICR_USE_YUANRONG_BACKEND_
    _instanceManager = HiCR::backend::yuanrong::L1::InstanceManager::createDefault(_pargc, _pargv);
#endif

    // Checking an instance manager has been found
    if (_instanceManager.get() == nullptr) HICR_THROW_LOGIC("No suitable backend for the instance manager was found.\n");

    /////////////////////////// Detecting topology managers

    // Clearing current topology managers (if any)
    _topologyManagers.clear();

// Detecting Hwloc
#ifdef _HICR_USE_HWLOC_BACKEND_
    _topologyManagers.push_back(std::move(HiCR::backend::host::hwloc::L1::TopologyManager::createDefault()));
#endif

// Detecting Ascend
#ifdef _HICR_USE_ASCEND_BACKEND_
    _topologyManagers.push_back(std::move(HiCR::backend::ascend::L1::TopologyManager::createDefault()));
#endif

    // Checking at least one topology manager was defined
    if (_topologyManagers.empty() == true) HICR_THROW_LOGIC("No suitable backends for topology managers were found.\n");

    /////////////////////////// Creating machine model object

    std::vector<HiCR::L1::TopologyManager *> topologyManagers;
    for (const auto &tm : _topologyManagers) topologyManagers.push_back(tm.get());
    _machineModel = std::make_unique<HiCR::MachineModel>(*_instanceManager, topologyManagers);
  }

  ~Runtime() = default;

  /**
   * This function initializes the HiCR runtime singleton and runs the backen detect and initialization routines
   *
   * @param[in] pargc A pointer to the argc value, as passed to main() necessary for new HiCR instances to count with argument information upon creation.
   * @param[in] pargv A pointer to the argv value, as passed to main() necessary for new HiCR instances to count with argument information upon creation.
   */
  static void initialize(int *pargc, char ***pargv) { _runtime = new Runtime(pargc, pargv); }

  /**
   * This function aborts execution, while trying to bring down all other instances (prevents hang ups). It may be only be called by the coordinator.
   * Calling this function from a worker may result in a hangup.
   *
   * @param[in] errorCode The error code to produce upon abortin execution
   */
  static void abort(const int errorCode) { _runtime->_abort(errorCode); }

  /**
   * Deploys the requested machine model and uses the a user-provided acceptance criteria function to evaluate whether the allotted resources satisfy the request
   *
   * @param[in] requests A vector of instance requests, expressing the requested system's machine model and the tasks that each instance needs to run
   * @param[in] acceptanceCriteriaFc A user-given function that compares the requested topology for a given instance and the one obtained to decide whether it meets the user requirements
   */
  static void deploy(std::vector<HiCR::MachineModel::request_t> &requests, HiCR::MachineModel::topologyAcceptanceCriteriaFc_t acceptanceCriteriaFc) { _runtime->_deploy(requests, acceptanceCriteriaFc, *_runtime->_pargc, *_runtime->_pargv); }

  /**
   * Adds a task that will be a possible target as initial function for a deployed HiCR instance.
   *
   * @param[in] taskName A human-readable string that defines the name of the task. To be executed, this should coincide with the name of a task specified in the machine model requests.
   * @param[in] fc Actual function to be executed upon instantiation
   */
  static void addTask(const std::string &taskName, const HiCR::L1::InstanceManager::RPCFunction_t fc) { return _runtime->_instanceManager->addRPCTarget(taskName, fc); }

  /**
   * Indicates whether the caller HiCR instance is the coordinator instance. There is only one coordiator instance per deployment.
   *
   * @return True, if the caller is the coordinator; false, otherwise.
   */
  static bool isCoordinator() { return _runtime->_instanceManager->getCurrentInstance()->isRootInstance(); }

  /**
   * Indicates whether the caller HiCR instance is a worker instance. There are n-1 workers in an n-sized deployment.
   *
   * @return True, if the caller is a worker; false, otherwise.
   */
  static bool isWorker() { return _runtime->_instanceManager->getCurrentInstance()->isRootInstance() == false; }

  /**
   * Blocking function that sets a worker to start listening for incoming RPCs
   */
  static void listen() { _runtime->_listen(); }

  /**
   * This function returns the unique numerical identifier for the caler instance
   *
   * @return An integer number containing the HiCR instance identifier
   */
  static HiCR::L0::Instance::instanceId_t getInstanceId() { return _runtime->_instanceManager->getCurrentInstance()->getId(); }

  /**
   * This function should be used at the end of execution by all HiCR instances, to correctly finalize the execution environment
   */
  static void finalize()
  {
    // Finalizing instance manager
    _runtime->_finalize();

    // Freeing up memory
    delete _runtime;
  }

  private:

  /**
   * Internal implementation of the finalize function
   */
  void _finalize()
  {
    // Finalizing instance manager
    _instanceManager->finalize();
  }

  /**
   * Internal implementation of the abort function
   */
  void _abort(const int errorCode = 0)
  {
    _instanceManager->abort(errorCode);
  }

  /**
   * Internal implementation of the listen function
   */
  void _listen()
  {
    // Flag to indicate whether the worker should continue listening
    bool continueListening = true;

    // Adding RPC targets, specifying a name and the execution unit to execute
    _instanceManager->addRPCTarget("Finalize", [&]()
                                   { continueListening = false; });

    // Listening for RPC requests
    while (continueListening == true) _instanceManager->listen();

    // Registering an empty return value to sync on finalization
    _instanceManager->submitReturnValue(nullptr, 0);

    // Finalizing
    _instanceManager->finalize();

    // Exiting
    exit(0);
  }

  /**
   * Internal implementation of the deploy function
   */
  void _deploy(std::vector<HiCR::MachineModel::request_t> &requests, HiCR::MachineModel::topologyAcceptanceCriteriaFc_t acceptanceCriteriaFc, int argc, char *argv[])
  {
    // Execute requests by finding or creating an instance that matches their topology requirements
    try
    {
      _machineModel->deploy(requests, acceptanceCriteriaFc);
    }
    catch (const std::exception &e)
    {
      fprintf(stderr, "Error while executing requests. Reason: '%s'\n", e.what());
      _abort(-1);
    }

    // Running the assigned task id in the correspondng instance
    for (auto &r : requests)
      for (auto &in : r.instances) _instanceManager->launchRPC(*in, r.taskName);

    // Now waiting for return values to arrive
    for (const auto &instance : _instanceManager->getInstances())
      if (instance->getId() != _instanceManager->getCurrentInstance()->getId())
        _instanceManager->launchRPC(*instance, "Finalize");

    // Now waiting for return values to arrive
    for (auto &r : requests)
      for (auto &in : r.instances) _instanceManager->getReturnValue(*in);
  }

  /**
   * Storage pointer for argc
   */
  int *const _pargc;

  /**
   * Storage pointer for argv
   */
  char ***const _pargv;

  /**
   * Detected instance manager to use for detecting and creating HiCR instances (only one allowed)
   */
  std::unique_ptr<HiCR::L1::InstanceManager> _instanceManager = nullptr;

  /**
   * Detected topology managers to use for resource discovery
   */
  std::vector<std::unique_ptr<HiCR::L1::TopologyManager>> _topologyManagers;

  /**
   * Storage for the detected instances
   */
  std::vector<std::unique_ptr<HiCR::runtime::Instance>> _instances;

  /**
   * Storage for the machine model instance
   */
  std::unique_ptr<HiCR::MachineModel> _machineModel;
};

} // namespace HiCR