
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

static Runtime* _runtime;

class Runtime final
{
  public:

  Runtime(int* argc, char*** argv) : _argc(argc), _argv(argv)
  {
    /////////////////////////// Detecting instance manager

    // Detecting MPI
    #ifdef _HICR_USE_MPI_BACKEND_

    // Creating instance manager
    _instanceManager = HiCR::backend::mpi::L1::InstanceManager::createDefault(_argc, _argv);

    #endif

    // Detecting YuanRong
    #ifdef _HICR_USE_YUANRONG_BACKEND_

    // Instantiating YuanRong instance manager
    _instanceManager = HiCR::backend::yuanrong::L1::InstanceManager::createDefault(_argc, _argv);

    #endif

    // Checking an instance manager has been found
    if (_instanceManager.get() == nullptr) HICR_THROW_LOGIC("No suitable backend for the instance manager was found.\n");

    /////////////////////////// Detecting topology managers

    // Clearing current topology managers (if any)
    _topologyManagers.clear();

    // Detecting Hwloc
    #ifdef _HICR_USE_HWLOC_BACKEND_

    // Adding topology manager to the list
    _topologyManagers.push_back(std::move(HiCR::backend::host::hwloc::L1::TopologyManager::createDefault()));

    #endif

    // Detecting Ascend
    #ifdef _HICR_USE_ASCEND_BACKEND_

     // Adding topology manager to the list
    _topologyManagers.push_back(std::move(HiCR::backend::ascend::L1::TopologyManager::createDefault()));

    #endif

    // Checking at least one topology manager was defined
    if (_topologyManagers.empty() == true) HICR_THROW_LOGIC("No suitable backends for topology managers were found.\n");

    /////////////////////////// Creating machine model object

    std::vector<HiCR::L1::TopologyManager*> topologyManagers;
    for (const auto& tm : _topologyManagers) topologyManagers.push_back(tm.get());
    _machineModel = std::make_unique<HiCR::MachineModel>(*_instanceManager, topologyManagers);
  }

  ~Runtime() = default;

  static void initialize(int* argc, char*** argv) { _runtime = new Runtime(argc, argv); }
  static void abort(const int errorCode) { _runtime->_abort(errorCode); }
  static void deploy(std::vector<HiCR::MachineModel::request_t> &requests, HiCR::MachineModel::topologyAcceptanceCriteriaFc_t acceptanceCriteriaFc) { _runtime->_deploy(requests, acceptanceCriteriaFc, *_runtime->_argc, *_runtime->_argv); }
  static void addTask(const std::string &RPCName, const HiCR::L1::InstanceManager::RPCFunction_t fc) { return _runtime->_instanceManager->addRPCTarget(RPCName, fc); }
  static bool isCoordinator() { return _runtime->_instanceManager->getCurrentInstance()->isRootInstance(); }
  static bool isWorker() { return _runtime->_instanceManager->getCurrentInstance()->isRootInstance() == false; }
  static void listen() { _runtime->_listen(); }

  static HiCR::L0::Instance::instanceId_t getInstanceId() { return _runtime->_instanceManager->getCurrentInstance()->getId(); }

  static void finalize()
  {
    // Finalizing instance manager
   _runtime->_finalize();

   // Freeing up memory
   delete _runtime;
  }

  private:

  void _finalize()
  {
    // Finalizing instance manager
   _instanceManager->finalize();
  }

  void _abort(const int errorCode = 0)
  {
    _instanceManager->abort(errorCode);
  }

  void _listen()
  {
    // Flag to indicate whether the worker should continue listening
    bool continueListening = true;

    // Adding RPC targets, specifying a name and the execution unit to execute
    _instanceManager->addRPCTarget("Finalize", [&]() { continueListening = false; });

    // Listening for RPC requests
    while (continueListening == true)  _instanceManager->listen();

     // Registering an empty return value to sync on finalization
     _instanceManager->submitReturnValue(nullptr, 0);

    // Finalizing
    _instanceManager->finalize();

    // Exiting
    exit(0);
  }

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
    for (auto &r : requests) for (auto &in : r.instances)  _instanceManager->launchRPC(*in, r.taskName);

    // Now waiting for return values to arrive
    for (const auto &instance : _instanceManager->getInstances()) 
      if (instance->getId() != _instanceManager->getCurrentInstance()->getId())
       _instanceManager->launchRPC(*instance, "Finalize");

    // Now waiting for return values to arrive
    for (auto &r : requests) for (auto &in : r.instances)  _instanceManager->getReturnValue(*in);
  }

  /**
   * Storage pointer for argc
  */
  int* const _argc;

  /**
   * Storage pointer for argv
  */
  char*** const _argv;

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