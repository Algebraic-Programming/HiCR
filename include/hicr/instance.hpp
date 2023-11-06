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
     * The instance is currently running
     */
    running,

    /**
     * The instance has reached its end
     */
    finished
  };

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
