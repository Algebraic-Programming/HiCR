/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file instanceManager.hpp
 * @brief Provides a definition for the abstract instance manager class
 * @author S. M. Martin
 * @date 2/11/2023
 */

#pragma once

#include <memory>
#include <set>
#include <map>
#include <hicr/instance.hpp>

namespace HiCR
{

namespace backend
{

/**
 * Encapsulates a HiCR Backend Instance Manager.
 *
 * Backends need to fulfill the abstract virtual functions described here, so that HiCR can detect/create/communicate with other HiCR instances
 *
 */
class InstanceManager
{
  public:

  /**
   * Default destructor
   */
  virtual ~InstanceManager() = default;

  /**
   * This function prompts the backend to perform the necessary steps to discover and list the currently created (active or not)
   */
  __USED__ inline const std::set<std::unique_ptr<HiCR::Instance>>& getInstances() const { return _instances; }

  /**
   * Function to add a new execution unit, assigned to a unique identifier
   */
  __USED__ inline void addExecutionUnit(const HiCR::Instance::executionUnitIndex_t index, HiCR::ExecutionUnit* executionUnit) { _executionUnitMap[index] = executionUnit; }

  /**
   * Function to add a new processing unit, assigned to a unique identifier
   */
  __USED__ inline void addProcessingUnit(const HiCR::Instance::processingUnitIndex_t index, std::unique_ptr<HiCR::ProcessingUnit> processingUnit) { _processingUnitMap[index] = std::move(processingUnit); }

  /**
   * Function to check whether the current instance is the coordinator one (or just a worker)
   */
  virtual bool isCoordinatorInstance() = 0;

  /**
   * Function to put the current instance to listen for incoming requests
   */
  virtual void listen() = 0;

  protected:

  __USED__ inline void runRequest(const HiCR::Instance::processingUnitIndex_t pIdx, const HiCR::Instance::executionUnitIndex_t eIdx)
  {
   // Checks that the processing and execution units have been registered
   if (_processingUnitMap.contains(pIdx) == false) HICR_THROW_RUNTIME("Attempting to run an processing unit (%lu) that was not defined in this instance (0x%lX).\n", pIdx, this);
   if (_executionUnitMap.contains(eIdx) == false) HICR_THROW_RUNTIME("Attempting to run an execution unit (%lu) that was not defined in this instance (0x%lX).\n", eIdx, this);

   // Getting units
   auto& p = _processingUnitMap[pIdx];
   auto& e = _executionUnitMap[eIdx];

   // Creating execution state
   auto s = p->createExecutionState(e);

   // Running execution state
   p->start(std::move(s));
  }

  /**
  * Protected constructor; this is a purely abstract class
  */
  InstanceManager() = default;

  /**
   * Collection of instances
   */
  std::set<std::unique_ptr<HiCR::Instance>> _instances;

  /**
   * Map of assigned processing units in charge of executing a execution units
   */
  std::map<HiCR::Instance::processingUnitIndex_t, std::unique_ptr<HiCR::ProcessingUnit>> _processingUnitMap;

  /**
   * Map of execution units, representing potential RPC requests
   */
  std::map<HiCR::Instance::executionUnitIndex_t, HiCR::ExecutionUnit*> _executionUnitMap;
};

} // namespace backend

} // namespace HiCR
