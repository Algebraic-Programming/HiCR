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
#include <hicr/L1/communicationManager.hpp>
#include <hicr/L1/topologyManager.hpp>
#include <frontends/machineModel/machineModel.hpp>
#include "worker.hpp"
#include "coordinator.hpp"

#ifdef _HICR_USE_MPI_BACKEND_
  #include <backends/mpi/L1/instanceManager.hpp>
  #include <backends/mpi/L1/communicationManager.hpp>
  #include <backends/mpi/L1/memoryManager.hpp>
  #include "dataObject/mpi/dataObject.hpp"
#endif

#ifdef _HICR_USE_YUANRONG_BACKEND_
  #include <backends/yuanrong/L1/instanceManager.hpp>
#endif

#ifdef _HICR_USE_HWLOC_BACKEND_
  #include <hwloc.h>
  #include <backends/host/hwloc/L1/topologyManager.hpp>
  #include <backends/host/hwloc/L1/memoryManager.hpp>
#endif

#ifdef _HICR_USE_ASCEND_BACKEND_
  #include <backends/ascend/L1/topologyManager.hpp>
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
   * The constructor for the HiCR runtime singleton takes the argc and argv pointers, as passed to the main() function
   *
   * @param[in] pargc Pointer to the argc argument count
   * @param[in] pargv Pointer to the argc argument char arrays
   */
  Runtime(int *pargc, char ***pargv)
    : _pargc(pargc),
      _pargv(pargv)
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

/////////////////////////// Detecting communication manager

// Detecting MPI
#ifdef _HICR_USE_MPI_BACKEND_
    _communicationManager = std::make_unique<HiCR::backend::mpi::L1::CommunicationManager>();
#endif

// Check if a communication manager is found if YuanRong is not used
#ifndef _HICR_USE_YUANRONG_BACKEND_
    if (_communicationManager.get() == nullptr) HICR_THROW_LOGIC("No suitable backend for the communication manager was found.\n");
#endif
/////////////////////////// Detecting memory manager

// Detecting HWLOC
#ifdef _HICR_USE_HWLOC_BACKEND_
    hwloc_topology_t topology;
    hwloc_topology_init(&topology);
    _memoryManager = std::make_unique<HiCR::backend::host::hwloc::L1::MemoryManager>(&topology);
#endif

// Detecting MPI
#ifdef _HICR_USE_MPI_BACKEND_
    _memoryManager = std::make_unique<HiCR::backend::mpi::L1::MemoryManager>();
#endif

    // Checking an instance manager has been found
    if (_memoryManager.get() == nullptr) HICR_THROW_LOGIC("No suitable backend for the memory manager was found.\n");

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

    // Creating machine model object
    std::vector<HiCR::L1::TopologyManager *> topologyManagers;
    for (const auto &tm : _topologyManagers) topologyManagers.push_back(tm.get());
    _machineModel = std::make_unique<HiCR::MachineModel>(*_instanceManager, topologyManagers);

    //////////////////////// Creating local HiCR Runtime instance, depending if worker or coordinator

    // Coordinator route
    if (_instanceManager->getCurrentInstance()->isRootInstance() == true) _currentInstance = new HiCR::runtime::Coordinator(*_instanceManager, *_communicationManager, *_memoryManager, topologyManagers, *_machineModel);

    // Worker route
    if (_instanceManager->getCurrentInstance()->isRootInstance() == false) _currentInstance = new HiCR::runtime::Worker(*_instanceManager, *_communicationManager, *_memoryManager, topologyManagers, *_machineModel);
  }

  ~Runtime() = default;

  /**
   * This function detects the backends that HiCR has been compiled against and creates the relevant L1 classes based on that.
   *
   * It also creates the machine model object for future deployment and decides whether this instance is coordinator or worker.
   */
  __USED__ inline void initialize()
  {
    // Executing delayed entry point registration
    for (const auto &entryPoint : _runtimeEntryPointVector) _instanceManager->addRPCTarget(entryPoint.first, entryPoint.second);

    // Initializing current instance
    _currentInstance->initialize();
  }

  /**
   * Returns the id of the coordinator instance
   *
   * @return The id of the coordinator instance
   */
  HiCR::L0::Instance::instanceId_t getCoordinatorInstanceId() const
  {
    return _instanceManager->getRootInstanceId();
  }

  /**
   * Returns a list of all the created instances id
   *
   * @return The list of the instances id
   */
  std::vector<HiCR::L0::Instance::instanceId_t> getInstanceIds() const
  {
    return _currentInstance->getInstanceIds();
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
  __USED__ inline void abort(const int errorCode)
  {
    if (_currentInstance == nullptr) HICR_THROW_LOGIC("Calling abort before HiCR has been initialized.\n");

    _instanceManager->abort(errorCode);
  }

  /**
   * Deploys the requested machine model and uses the a user-provided acceptance criteria function to evaluate whether the allotted resources satisfy the request
   *
   * @param[in] requests A vector of instance requests, expressing the requested system's machine model and the tasks that each instance needs to run
   * @param[in] acceptanceCriteriaFc A user-given function that compares the requested topology for a given instance and the one obtained to decide whether it meets the user requirements
   */
  __USED__ inline void deploy(std::vector<HiCR::MachineModel::request_t> &requests, HiCR::MachineModel::topologyAcceptanceCriteriaFc_t acceptanceCriteriaFc)
  {
    if (_currentInstance == nullptr) HICR_THROW_LOGIC("Calling deploy before HiCR has been initialized.\n");

    // Calling coordinator's deploy function
    dynamic_cast<HiCR::runtime::Coordinator *>(_currentInstance)->deploy(requests, acceptanceCriteriaFc, *_pargc, *_pargv);
  }

  /**
   * Adds a task that will be a possible target as initial function for a deployed HiCR instance.
   *
   * @param[in] entryPointName A human-readable string that defines the name of the task. To be executed, this should coincide with the name of a task specified in the machine model requests.
   * @param[in] fc Actual function to be executed upon instantiation
   */
  __USED__ inline void registerEntryPoint(const std::string &entryPointName, const HiCR::L1::InstanceManager::RPCFunction_t fc) { _runtimeEntryPointVector.push_back(entryPoint_t(entryPointName, fc)); }

  /**
   * This function returns the unique numerical identifier for the caler instance
   *
   * @return An integer number containing the HiCR instance identifier
   */
  __USED__ inline HiCR::L0::Instance::instanceId_t getInstanceId()
  {
    if (_currentInstance == nullptr) HICR_THROW_LOGIC("Calling getInstanceId before HiCR has been initialized.\n");

    return _currentInstance->getHiCRInstance()->getId();
  }

  /**
   * This function should be used at the end of execution by all HiCR instances, to correctly finalize the execution environment
   */
  __USED__ inline void finalize()
  {
    if (_currentInstance == nullptr) HICR_THROW_LOGIC("Calling finalize before HiCR has been initialized.\n");

    // Initializing current instance
    _currentInstance->finalize();

    // Freeing up instance memory
    delete _currentInstance;
  }

  private:

  /**
   * Storage pointer for argc
   */
  int *const _pargc;

  /**
   * Storage pointer for argv
   */
  char ***const _pargv;

  /**
   * The entry point type is the combination of its name (a string) and the associated function to execute
   */
  typedef std::pair<const std::string, const HiCR::L1::InstanceManager::RPCFunction_t> entryPoint_t;

  /**
   * A temporary storage place for entry points so that workers can register them before initializing (and therefore losing control over their execution)
   */
  std::vector<entryPoint_t> _runtimeEntryPointVector;

  /**
   * The currently running instance
   */
  HiCR::runtime::Instance *_currentInstance = nullptr;

  /**
   * Detected instance manager to use for detecting and creating HiCR instances (only one allowed)
   */
  std::unique_ptr<HiCR::L1::InstanceManager> _instanceManager = nullptr;

  /**
   * Detected communication manager to use for communicating between HiCR instances
   */
  std::unique_ptr<HiCR::L1::CommunicationManager> _communicationManager = nullptr;

  /**
   * Detected memory manager to use for allocating memory
   */
  std::unique_ptr<HiCR::L1::MemoryManager> _memoryManager = nullptr;

  /**
   * Detected topology managers to use for resource discovery
   */
  std::vector<std::unique_ptr<HiCR::L1::TopologyManager>> _topologyManagers;

  /**
   * Machine model object for deployment
   */
  std::unique_ptr<HiCR::MachineModel> _machineModel;
};

} // namespace HiCR
