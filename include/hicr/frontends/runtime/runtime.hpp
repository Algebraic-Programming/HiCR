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
#include <hicr/core/exceptions.hpp>
#include <hicr/core/L1/instanceManager.hpp>
#include <hicr/core/L1/memoryManager.hpp>
#include <hicr/core/L1/communicationManager.hpp>
#include <hicr/core/L1/topologyManager.hpp>
#include <hicr/frontends/machineModel/machineModel.hpp>
#include <hicr/backends/host/L1/instanceManager.hpp>
#include "instance.hpp"

#ifdef _HICR_USE_MPI_BACKEND_
  #include "dataObject/mpi/dataObject.hpp"
#endif

namespace HiCR
{

/**
 * The runtime class represents a singleton that exposes many HiCR's (and its frontends') functionalities with a simplified API
 * and performs backend detect and initialization of a selected set of backends. This class goal is to be FF3 RTS's backend library
 * but it can used by other libraries as well.
 */
class Runtime final
{
  public:

  /**
   * Constructor for the HiCR Runtime class
   *
   * @param[in] instanceManager The instance manager backend to use
   * @param[in] communicationManager The communication manager backend to use
   * @param[in] memoryManager The memory manager backend to use
   * @param[in] topologyManagers The topology managers backend to use to discover the system's resources
   * @param[in] machineModel The machine model to use to deploy the workers
   */
  Runtime(    HiCR::L1::InstanceManager      *instanceManager,
              HiCR::L1::CommunicationManager *communicationManager,
              HiCR::L1::MemoryManager        *memoryManager,
              std::vector<HiCR::L1::TopologyManager*>&   topologyManagers) :
              _instanceManager(instanceManager)
  {
    // Creating machine model object
    _machineModel = std::make_unique<HiCR::MachineModel>(*_instanceManager, topologyManagers);

    // Creating local HiCR Runtime instance
    _currentInstance = new HiCR::runtime::Instance(instanceManager, communicationManager, memoryManager, topologyManagers, _machineModel.get());
  }

  ~Runtime() = default;

  /**
   * This function detects the backends that HiCR has been compiled against and creates the relevant L1 classes based on that.
   *
   * It also creates the machine model object for future deployment and decides whether this instance is coordinator or worker.
   */
  __INLINE__ void initialize()
  {
    // Executing delayed entry point registration
    for (const auto &entryPoint : _runtimeEntryPointVector) _instanceManager->addRPCTarget(entryPoint.first, entryPoint.second);

    // If this is not the root instance, then start listening for RPC requests
    if (_currentInstance->getHiCRInstance()->getId() != _instanceManager->getRootInstanceId()) _currentInstance->listen();
  }

  /**
   * Gets a pointer to the currently running runtime instance
   *
   * @return A pointer to the current instance
   */
  HiCR::runtime::Instance *getCurrentInstance() const
  {
    // Sanity check
    if (_currentInstance == nullptr) HICR_THROW_LOGIC("Calling getCoordinatorInstance before HiCR has been initialized.\n");

    return _currentInstance;
  }

  /**
   * This function aborts execution, while trying to bring down all other instances (prevents hang ups). It may be only be called by the coordinator.
   * Calling this function from a worker may result in a hangup.
   *
   * @param[in] errorCode The error code to produce upon abortin execution
   */
  __INLINE__ void abort(const int errorCode)
  {
    if (_currentInstance == nullptr) HICR_THROW_LOGIC("Calling abort before HiCR has been initialized.\n");
    _instanceManager->abort(errorCode);
  }

  /**
   * Deploys the requested machine model and uses the a user-provided acceptance criteria function to evaluate whether the allotted resources satisfy the request
   *
   * @param[in] requests A vector of instance requests, expressing the requested system's machine model and the tasks that each instance needs to run
   * @param[in] acceptanceCriteriaFc A user-given function that compares the requested topology for a given instance and the one obtained to decide whether it meets the user requirements
   * @param[in] argc The number of command line arguments
   * @param[in] argv Poiners to the character string for the command line arguments
   */
  __INLINE__ void deploy(std::vector<HiCR::MachineModel::request_t> &requests, HiCR::MachineModel::topologyAcceptanceCriteriaFc_t acceptanceCriteriaFc, int argc = 0, char *argv[] = nullptr)
  {
    if (_currentInstance == nullptr) HICR_THROW_LOGIC("Calling deploy before HiCR has been initialized.\n");

    // Execute requests by finding or creating an instance that matches their topology requirements
    try { _machineModel->deploy(requests, acceptanceCriteriaFc, argc, argv); }
    catch (const std::exception &e) 
    {
      fprintf(stderr, "Error while executing deployment requests. Reason: '%s'\n", e.what());
      abort(exceptions::exception_t::runtime);
    }

    // Registering new deployed instances and launching channel initialization procedure
    for (auto &request : requests)
      for (auto &instance : request.instances)
       if (instance->getId() != _currentInstance->getHiCRInstance()->getId())
      {
        // Registering instance
        _deployedInstances.push_back(instance);

        // Launching channel creation routine for deployed instance
        _instanceManager->launchRPC(*instance, "__initializeChannels");
      }
        
    // Initializing channel for the current instance
    _currentInstance->initializeChannels();

    // Launching other's instances entry point function first, remembering our own entry point
    std::string currentInstanceEntryPoint;
    for (auto &request : requests)
     for (auto &instance : request.instances)
      if (instance->getId() != _currentInstance->getHiCRInstance()->getId())
       _instanceManager->launchRPC(*instance, request.entryPointName);
      else
       currentInstanceEntryPoint = request.entryPointName;

    // Running current instance's own entry point, if defined
    if (currentInstanceEntryPoint != "") 
    {
      // Calculating hash for the RPC target's name
      auto idx = _instanceManager->getRPCTargetIndexFromString(currentInstanceEntryPoint);

      // Running entry point
      _instanceManager->executeRPC(idx);
    }
  }

  /**
   * Adds a task that will be a possible target as initial function for a deployed HiCR instance.
   *
   * @param[in] entryPointName A human-readable string that defines the name of the task. To be executed, this should coincide with the name of a task specified in the machine model requests.
   * @param[in] fc Actual function to be executed upon instantiation
   */
  __INLINE__ void registerEntryPoint(const std::string &entryPointName, const HiCR::L1::InstanceManager::RPCFunction_t fc)
  {
    _runtimeEntryPointVector.push_back(entryPoint_t(entryPointName, fc));
  }

  /**
   * This function returns the unique numerical identifier for the caler instance
   *
   * @return An integer number containing the HiCR instance identifier
   */
  __INLINE__ HiCR::L0::Instance::instanceId_t getInstanceId()
  {
    if (_currentInstance == nullptr) HICR_THROW_LOGIC("Calling getInstanceId before HiCR has been initialized.\n");

    return _currentInstance->getHiCRInstance()->getId();
  }

  /**
   * This function should be used at the end of execution by all HiCR instances, to correctly finalize the execution environment
   */
  __INLINE__ void finalize()
  {
    if (_currentInstance == nullptr) HICR_THROW_LOGIC("Calling finalize before HiCR has been initialized.\n");

    // Launching finalization RPC on all deployed instances
    for (auto &instance : _deployedInstances)
     if (instance->getId() != _currentInstance->getHiCRInstance()->getId())
      _instanceManager->launchRPC(*instance, "__finalize");

    // Waiting for return ack
    for (auto &instance : _deployedInstances)
     if (instance->getId() != _currentInstance->getHiCRInstance()->getId())
      _instanceManager->getReturnValue(*instance);

    // Finalizing instance manager
    _instanceManager->finalize();

    // Freeing up instance memory
    delete _currentInstance;
  }

  __INLINE__ HiCR::L1::InstanceManager* getInstanceManager() { return _instanceManager; }

  private:

  /**
   * The entry point type is the combination of its name (a string) and the associated function to execute
   */
  typedef std::pair<const std::string, const HiCR::L1::InstanceManager::RPCFunction_t> entryPoint_t;

  /**
   * A temporary storage place for entry points so that workers can register them before initializing (and therefore losing control over their execution)
   */
  std::vector<entryPoint_t> _runtimeEntryPointVector;

  /**
   * Machine model object for deployment
   */
  std::unique_ptr<HiCR::MachineModel> _machineModel;

   /**
   * The currently running instance
   */
  HiCR::runtime::Instance *_currentInstance = nullptr;

   /**
   * Instance manager to use for detecting and creating HiCR instances (only one allowed)
   */
  HiCR::L1::InstanceManager* const _instanceManager;

  /**
   * Storage for the deployed instances. This object is only mantained and usable by the coordinator instance
   */
  std::vector<std::shared_ptr<HiCR::L0::Instance>> _deployedInstances;
};

} // namespace HiCR
