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
     * The instance is online but not listening (detached mode)
     */
    detached,

    /**
     * The instance is currently running
     */
    running,

    /**
     * The instance is listening for incoming RPCs (attached)
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
  __USED__ inline void listen()
  {
   // Setting current state to listening
   _state = state_t::listening;

   // Calling the backend-specific implementation of the listen function
   listenImpl();

   // Setting current state to detached
   _state = state_t::detached;
  }

  /**
   * Function to trigger the execution of a remote function in a remote HiCR instance
   */
  virtual void execute(const processingUnitIndex_t pIdx, const executionUnitIndex_t eIdx) = 0;

  /**
   * Function to submit a return value for the currently running RPC
   */
  __USED__ inline void submitReturnValue(const uint8_t* data, const size_t size)
  {
   if (_state != state_t::running) HICR_THROW_LOGIC("Attempting to submit a return value outside a running RPC.");

   // Calling backend-specific implementation of this function
   submitReturnValueImpl(data, size);
  }

  /**
   * Function to get a return value from a remote instance that ran an RPC
   */
  __USED__ inline size_t getReturnValueSize()
  {
   if (getState() != state_t::running) HICR_THROW_LOGIC("Attempting to get a return value size from a non-running instance.");

   // Calling backend-specific implementation of this function
   return getReturnValueSizeImpl();
  }

  /**
   * Backend-specific implementation of the getReturnValueData function
   */
  __USED__ inline void getReturnValueData(uint8_t* data, const size_t size)
  {
   // Calling backend-specific implementation of this function
   return getReturnValueDataImpl(data, size);
  }


  /**
   * State getter
   */
  virtual state_t getState() const = 0;

  /**
   * Convenience function to print the state as a string
   */
  static std::string getStateString(const state_t state)
  {
    if (state == state_t::detached)  return std::string("Detached");
    if (state == state_t::listening) return std::string("Listening");
    if (state == state_t::running)   return std::string("Running");
    if (state == state_t::finished)  return std::string("Finished");

    HICR_THROW_LOGIC("Unrecognized instance state (0x%lX).\n", state);
  }

  protected:

  /**
   * Backend-specific implementation of the getReturnValue function
   */
  virtual size_t getReturnValueSizeImpl() = 0;

  /**
   * Backend-specific implementation of the getReturnValueData function
   */
  virtual void getReturnValueDataImpl(uint8_t* data, const size_t size) = 0;

  /**
   * Backend-specific implementation of the submitReturnValue
   */
  virtual void submitReturnValueImpl(const uint8_t* data, const size_t size) = 0;

  /**
   * Backend-specific implementation of the listen function
   */
  virtual void listenImpl() = 0;

  __USED__ inline void runRequest(const HiCR::Instance::processingUnitIndex_t pIdx, const HiCR::Instance::executionUnitIndex_t eIdx)
  {
   // Setting current state to running
   _state = state_t::running;

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

   // Setting current state to detached
   _state = state_t::detached;
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
  state_t _state = state_t::detached;

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
