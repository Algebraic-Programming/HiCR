/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file instance.hpp
 * brief Provides a definition for the HiCR Instance class.
 * @author S. M. Martin
 * @date 2/11/2023
 */

#pragma once

#include <hicr/common/definitions.hpp>
#include <hicr/common/exceptions.hpp>
#include <hicr/executionUnit.hpp>
#include <hicr/processingUnit.hpp>

namespace HiCR
{

/**
 * Defines the instance class, which represents a self-contained instance of HiCR with access to compute and memory resources.
 *
 * Instances may be created during runtime (if the process managing backend allows for it), or activated/suspended on demand.
 */
class Instance
{
  public:

 /**
  * Type definition for an index to identify a HiCR instance
  */
 typedef uint64_t instanceIndex_t;

 /**
  * Type definition for an index to indicate the execution of a specific execution unit
  */
 typedef uint64_t executionUnitIndex_t;

 /**
  * Type definition for an index to indicate the use of a specific processing unit in charge of executing a execution units
  */
 typedef uint64_t processingUnitIndex_t;

 /**
  * Function to add a new execution unit, assigned to a unique identifier
  */
 __USED__ inline void addExecutionUnit(const HiCR::Instance::executionUnitIndex_t index, HiCR::ExecutionUnit* executionUnit) { _executionUnitMap[index] = executionUnit; }

 /**
  * Function to add a new processing unit, assigned to a unique identifier
  */
 __USED__ inline void addProcessingUnit(const HiCR::Instance::processingUnitIndex_t index, std::unique_ptr<HiCR::ProcessingUnit> processingUnit) { _processingUnitMap[index] = std::move(processingUnit); }


  virtual ~Instance() = default;

  /**
   * Complete state set that a worker can be in
   */
  enum state_t
  {
    /**
     * The instance is currently running
     */
    running,

    /**
     * The instance is listening for incoming RPCs
     */
    listening,

    /**
     * The instance has reached its end
     */
    finished
  };

  /**
   * Function to put the current instance to listen for incoming requests
   */
  virtual void listen() = 0;

  /**
   * Function to invoke the execution of a remote function in a remote HiCR instance
   */
  virtual void invoke(const processingUnitIndex_t pIdx, const executionUnitIndex_t eIdx) = 0;

  /**
   * State getter
   */
  virtual state_t getState() const = 0;


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
   * State setter. Only used internally to update the state
   */
  __USED__ inline void setState(const state_t state) { _state = state; }


  /**
   * Protected constructor for the worker class.
   *
   * This is a purely abstract class
   */
  Instance() = default;

  /**
   * Represents the internal state of the instance. Inactive upon creation.
   */
  state_t _state = state_t::running;

  private:

  /**
   * Map of assigned processing units in charge of executing a execution units
   */
  std::map<HiCR::Instance::processingUnitIndex_t, std::unique_ptr<HiCR::ProcessingUnit>> _processingUnitMap;

  /**
   * Map of execution units, representing potential RPC requests
   */
  std::map<HiCR::Instance::executionUnitIndex_t, HiCR::ExecutionUnit*> _executionUnitMap;

};

} // namespace HiCR
