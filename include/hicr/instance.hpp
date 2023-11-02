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

  /**
   * Constructor for the worker class.
   *
   * \param[in] computeManager A backend's compute manager, meant to initialize and run the task's execution states.
   */
  Worker(HiCR::backend::ComputeManager *computeManager) : _computeManager(computeManager)
  {
    // Making sure the worker-identifying key is created (only once) with the first created task
    pthread_once(&_workerPointerKeyConfig, createWorkerPointerKey);
  }

  ~Worker() = default;

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


  private:

  /**
   * Represents the internal state of the instance. Inactive upon creation.
   */
  state_t _state = state_t::inactive;

};

} // namespace HiCR
