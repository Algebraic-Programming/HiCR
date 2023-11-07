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


  virtual ~Instance() = default;

  /**
   * Complete state set that a worker can be in
   */
  enum state_t
  {
    /**
     * The instance is created but currently inactive (waiting for something to do)
     */
    inactive,

    /**
     * The instance is listening for incoming RPCs
     */
    listening,

    /**
     * The instance is currently running
     */
    running,

    /**
     * The instance has reached its end
     */
    finished
  };

  /**
   * Function to invoke the execution of a remote function in a remote HiCR instance
   */
  virtual void invoke(const processingUnitIndex_t pIdx, const executionUnitIndex_t eIdx) = 0;

  /**
   * State getter
   */
  __USED__ inline state_t getState() const { return _state; }

  /**
   * State setter
   */
  __USED__ inline void setState(const state_t state) { _state = state; }

  protected:

  /**
   * Protected constructor for the worker class.
   *
   * This is a purely abstract class
   */
  Instance() = default;

  private:

  /**
   * Represents the internal state of the instance. Inactive upon creation.
   */
  state_t _state = state_t::inactive;

};

} // namespace HiCR
