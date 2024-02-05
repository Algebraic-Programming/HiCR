
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

  Runtime()
  {
    /////////////////////////// Detecting instance manager
    bool instanceManagerDetected = false;

    // Detecting MPI
    #ifdef _HICR_USE_MPI_BACKEND_
    { 
      // Checking the instance manager has not yet been defined
      if (instanceManagerDetected == true) HICR_THROW_LOGIC("Trying to initialize instance manager backend when another has already been defined.\n");

      // Initializing MPI
      int requested = MPI_THREAD_SERIALIZED;
      int provided;
      int argc = 0;
      char** argv = nullptr;
      MPI_Init_thread(&argc, &argv, requested, &provided);
      if (provided < requested) fprintf(stderr, "Warning, this example may not work properly if MPI does not support (serialized) threaded access\n");

      // Creating instance manager
      _instanceManager = std::make_unique<HiCR::backend::mpi::L1::InstanceManager>(MPI_COMM_WORLD);

      // Setting detected flag
      instanceManagerDetected = true;
    }
    #endif

    // Checking an instance manager has been found
    if (instanceManagerDetected == false) HICR_THROW_LOGIC("No suitable backend for the instance manager was found.\n");

    /////////////////////////// Detecting topology managers

    // Clearing current topology managers (if any)
    _topologyManagers.clear();

    // Detecting Hwloc
    #ifdef _HICR_USE_HWLOC_BACKEND_
    {
      // Creating HWloc topology object
      auto topology = new hwloc_topology_t;

      // Reserving memory for hwloc
      hwloc_topology_init(topology);

      // Initializing HWLoc-based host (CPU) topology manager
      auto tm = std::make_unique<HiCR::backend::host::hwloc::L1::TopologyManager>(topology);

      // Adding topology manager to the list
      _topologyManagers.push_back(std::move(tm));
    }
    #endif

    // Detecting Ascend
    #ifdef _HICR_USE_ASCEND_BACKEND_

    {
      // Initialize (Ascend's) ACL runtime
      aclError err = aclInit(NULL);
      if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Failed to initialize Ascend Computing Language. Error %d", err);

      // Initializing ascend topology manager
      auto tm = std::make_unique<HiCR::backend::ascend::L1::TopologyManager>();

       // Adding topology manager to the list
      _topologyManagers.push_back(std::move(tm));
    }

    #endif

    // Checking at least one topology manager was defined
    if (_topologyManagers.empty() == true) HICR_THROW_LOGIC("No suitable backends for topology managers were found.\n");

    /////////////////////////// Creating machine model object

    std::vector<HiCR::L1::TopologyManager*> topologyManagers;
    for (const auto& tm : _topologyManagers) topologyManagers.push_back(tm.get());
    _machineModel = std::make_unique<HiCR::MachineModel>(*_instanceManager, topologyManagers);
  }

  ~Runtime() = default;

  static void initialize() { _runtime = new Runtime(); }
  static void finalize()
  {
    // Finalizing instance manager
   _runtime->_instanceManager->finalize();

   // Freeing up memory
   delete _runtime;
  }

  private:

  /**
   * Detected instance manager to use for detecting and creating HiCR instances (only one allowed)
  */ 
  std::unique_ptr<HiCR::L1::InstanceManager> _instanceManager; 

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