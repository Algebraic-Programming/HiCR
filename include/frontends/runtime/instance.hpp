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

#include <vector>
#include <hicr/L1/instanceManager.hpp>
#include <hicr/L1/memoryManager.hpp>
#include <hicr/L1/communicationManager.hpp>
#include <hicr/L1/topologyManager.hpp>
#include <frontends/machineModel/machineModel.hpp>

namespace HiCR
{

namespace runtime
{

/**
 * Defines the worker class, which represents a self-contained instance of HiCR with access to compute and memory resources.
 *
 * Workers may be created during runtime (if the process managing backend allows for it), or activated/suspended on demand.
 */
class Instance
{
  public:


  Instance() = delete;
  Instance(
    HiCR::L1::InstanceManager& instanceManager,
    HiCR::L1::CommunicationManager& communicationManager,
    HiCR::L1::MemoryManager& memoryManager,
    std::vector<HiCR::L1::TopologyManager*>& topologyManagers,
    HiCR::MachineModel& machineModel
  ) :
    _HiCRInstance(instanceManager.getCurrentInstance()),
    _instanceManager(&instanceManager),
    _communicationManager(&communicationManager),
    _memoryManager(&memoryManager),
    _topologyManagers(topologyManagers),
    _machineModel(&machineModel)
   {
   }
  virtual ~Instance() = default;

  virtual void initialize() = 0;

  /**
  * This function should be used at the end of execution by all HiCR instances, to correctly finalize the execution environment
  */
  virtual void finalize() = 0;
  
  HiCR::L0::Instance* getHiCRInstance() const { return _HiCRInstance.get(); }

  protected:

  /**
   * Internal implementation of the abort function
   */
  void _abort(const int errorCode = 0)
  {
    _instanceManager->abort(errorCode);
  }
  
  /**
   * Internal HiCR instance represented by this runtime instance
  */
  const std::shared_ptr<HiCR::L0::Instance> _HiCRInstance;

  /*
   * Detected instance manager to use for detecting and creating HiCR instances (only one allowed)
   */
  HiCR::L1::InstanceManager* const _instanceManager = nullptr;

  /**
   * Detected communication manager to use for communicating between HiCR instances
   */
  HiCR::L1::CommunicationManager* const _communicationManager = nullptr;

  /**
   * Detected memory manager to use for allocating memory
   */
  HiCR::L1::MemoryManager* const _memoryManager;

  /**
   * Detected topology managers to use for resource discovery
   */
  std::vector<HiCR::L1::TopologyManager*> _topologyManagers;

   /**
   * Machine model object for deployment
   */
  HiCR::MachineModel* const _machineModel;
  
};

} // namespace runtime

} // namespace HiCR
