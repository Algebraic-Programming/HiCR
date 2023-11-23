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
#include <hicr/memorySlot.hpp>
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
   * Type definition for a unique instance identifier
   */
  typedef uint64_t instanceId_t;

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
   * \param[in] index Indicates the index to assign to the added execution unit
   * \param[in] executionUnit The execution unit to add
   */
  __USED__ inline void addExecutionUnit(const HiCR::Instance::executionUnitIndex_t index, HiCR::ExecutionUnit *executionUnit) { _executionUnitMap[index] = executionUnit; }

  /**
   * Function to add a new processing unit, assigned to a unique identifier
   * \param[in] index Indicates the index to assign to the added processing unit
   * \param[in] processingUnit The processing unit to add
   */
  __USED__ inline void addProcessingUnit(const HiCR::Instance::processingUnitIndex_t index, std::unique_ptr<HiCR::ProcessingUnit> processingUnit) { _processingUnitMap[index] = std::move(processingUnit); }

  /**
   * Default destructor
   */
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
   * \param[in] pIdx Index to the processing unit to use
   * \param[in] eIdx Index to the execution unit to run
   */
  virtual void execute(const processingUnitIndex_t pIdx, const executionUnitIndex_t eIdx) = 0;

  /**
   * Function to submit a return value for the currently running RPC
   * \param[in] value The memory slot containing the RPC's return value
   */
  __USED__ inline void submitReturnValue(HiCR::MemorySlot *value)
  {
    if (_state != state_t::running) HICR_THROW_LOGIC("Attempting to submit a return value outside a running RPC.");

    // Calling backend-specific implementation of this function
    submitReturnValueImpl(value);
  }

  /**
   * Function to get a return value from a remote instance that ran an RPC
   * \return A pointer to a newly allocated local memory slot containing the return value
   */
  __USED__ inline HiCR::MemorySlot *getReturnValue()
  {
    // Calling backend-specific implementation of this function
    return getReturnValueImpl();
  }

  /**
   * State getter
   * \return The internal state of the instance
   */
  virtual state_t getState() const = 0;

  /**
   * Convenience function to print the state as a string
   * \param[in] state The value of the state (enumeration)
   * \return The string corresponding to the passed state
   */
  static std::string getStateString(const state_t state)
  {
    if (state == state_t::detached) return std::string("Detached");
    if (state == state_t::listening) return std::string("Listening");
    if (state == state_t::running) return std::string("Running");
    if (state == state_t::finished) return std::string("Finished");

    HICR_THROW_LOGIC("Unrecognized instance state (0x%lX).\n", state);
  }

  __USED__ inline instanceId_t getId() const { return _id; }

  protected:

  /**
   * Backend-specific implementation of the getReturnValue function
   * \return A pointer to a newly allocated local memory slot containing the return value
   */
  virtual HiCR::MemorySlot *getReturnValueImpl() = 0;

  /**
   * Backend-specific implementation of the submitReturnValue
   * \param[in] value The memory slot containing the RPC's return value
   */
  virtual void submitReturnValueImpl(HiCR::MemorySlot *value) = 0;

  /**
   * Backend-specific implementation of the listen function
   */
  virtual void listenImpl() = 0;

  /**
   * Internal function used to initiate the execution of the requested RPC  bt running executionUnit using the indicated procesing unit
   * \param[in] pIdx Index to the processing unit to use
   * \param[in] eIdx Index to the execution unit to run
   */
  __USED__ inline void runRequest(const HiCR::Instance::processingUnitIndex_t pIdx, const HiCR::Instance::executionUnitIndex_t eIdx)
  {
    // Setting current state to running
    _state = state_t::running;

    // Checks that the processing and execution units have been registered
    if (_processingUnitMap.contains(pIdx) == false) HICR_THROW_RUNTIME("Attempting to run an processing unit (%lu) that was not defined in this instance (0x%lX).\n", pIdx, this);
    if (_executionUnitMap.contains(eIdx) == false) HICR_THROW_RUNTIME("Attempting to run an execution unit (%lu) that was not defined in this instance (0x%lX).\n", eIdx, this);

    // Getting units
    auto &p = _processingUnitMap[pIdx];
    auto &e = _executionUnitMap[eIdx];

    // Creating execution state
    auto s = p->createExecutionState(e);

    // Running execution state
    p->start(std::move(s));

    // Setting current state to detached
    _state = state_t::detached;
  }

  /**
   * State setter. Only used internally to update the state
   * \param[in] state The new value of the state to be set
   */
  __USED__ inline void setState(const state_t state) { _state = state; }

  /**
   * Protected constructor for the worker class.
   *
   * This is a purely abstract class
   */
  __USED__ Instance(instanceId_t id) : _id (id) {};


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
  std::map<HiCR::Instance::executionUnitIndex_t, HiCR::ExecutionUnit *> _executionUnitMap;

  /**
   * Instance Identifier
   */
  const instanceId_t _id;

};

} // namespace HiCR
