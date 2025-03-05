/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file processingUnit.hpp
 * @brief nOS-V processing unit class. Main job is to to submit the execution state task.
 * @author N. Baumann
 * @date 24/02/2025
 */
#pragma once

#include <memory>
#include <nosv.h>
#include <nosv/affinity.h>
#include <hicr/core/definitions.hpp>
#include <hicr/backends/hwloc/L0/computeResource.hpp>
#include <hicr/core/L0/processingUnit.hpp>
#include <utility>

#include <hicr/backends/nosv/L0/executionState.hpp>
#include <hicr/backends/nosv/common.hpp>

namespace HiCR::backend::nosv::L1
{
class ComputeManager;
}

namespace HiCR::backend::nosv::L0
{

/**
 * This class represents an abstract definition for a Processing Unit resource in HiCR that:
 *
 * - Represents a single compute resource that has been instantiated for execution (as opposed of those who shall remain unused or unassigned).
 * - Is capable of executing or contributing to the execution of tasks.
 * - Is assigned, for example, to a worker to perform the work necessary to execute a task.
 * - This is a non-copy-able class
 */
class ProcessingUnit final : public HiCR::L0::ProcessingUnit
{
  friend class HiCR::backend::nosv::L1::ComputeManager;

  public:

  /**
   * Constructor for a processing unit
   *
   * \param computeResource The instance of the compute resource to instantiate, as indicated by the backend
   */
  __INLINE__ ProcessingUnit(const std::shared_ptr<HiCR::L0::ComputeResource> &computeResource)
    : HiCR::L0::ProcessingUnit(computeResource)
  {
    // Getting up-casted pointer for the processing unit
    auto c = dynamic_pointer_cast<HiCR::backend::hwloc::L0::ComputeResource>(computeResource);

    // Checking whether the execution unit passed is compatible with this backend
    if (c == nullptr) HICR_THROW_LOGIC("The passed compute resource is not supported by this processing unit type\n");

    // move ownership to this instance
    _computeResource = std::move(c);
  };

  /**
   * Gets the processing unit's type
   *
   * \return A string, containing the processing unit's type
   */
  [[nodiscard]] __INLINE__ std::string getType() override { return "nOS-V worker thread"; }

  private:

  /**
   * 
   */
  __INLINE__ void initialize()
  {
    // Getting the logical processor ID of the compute resource
    auto pid = _computeResource->getProcessorId();

    // setting up the nosv affinity for the execution task
    _nosv_affinity = nosv_affinity_get(pid, NOSV_AFFINITY_LEVEL_CPU, NOSV_AFFINITY_TYPE_STRICT);
  }

  /**
   * Internal implementation of the resume function
   */
  __INLINE__ void suspend() { HICR_THROW_RUNTIME("nOS-V can't suspend a worker thread.\n"); }

  /**
   * Internal implementation of the resume function
   */
  __INLINE__ void resume() { HICR_THROW_RUNTIME("nOS-V can't resume a worker thread.\n"); }

  /**
   * Internal implementation of the start function
   */
  __INLINE__ void start(std::unique_ptr<HiCR::L0::ExecutionState> &executionState)
  {
    // Getting up-casted pointer of the execution state
    auto c = std::unique_ptr<HiCR::backend::nosv::L0::ExecutionState>(dynamic_cast<HiCR::backend::nosv::L0::ExecutionState *>(executionState.get()));

    // Checking whether the execution state passed is compatible with this backend
    if (c == nullptr) HICR_THROW_LOGIC("The passed execution state is not supported by this processing unit type\n");

    // Prevent double deletion since c now owns the pointer
    executionState.release();

    // Move the ovnership of the execution state to this instance
    _executionState = std::move(c);

    // Set execution statte task metadata for this PU
    auto metadata      = (ExecutionState::taskMetadata_t *)getTaskMetadata(_executionState->_executionStateTask);
    metadata->mainLoop = true;

    // Initialize the barrier
    check(nosv_barrier_init(&(metadata->mainLoop_barrier), NOSV_BARRIER_NONE, 2));

    // Set the task affinity
    nosv_set_task_affinity(_executionState->_executionStateTask, &_nosv_affinity);

    // Submit the job (Now, nosv will put it inside a queue and runs it ASAP)
    check(nosv_submit(_executionState->_executionStateTask, NOSV_SUBMIT_NONE));

    // Barrier to as we have to wait until the execution state task is properly initialized and running
    check(nosv_barrier_wait(metadata->mainLoop_barrier));
  }

  /**
   * Internal implementation of the terminate function
   */
  __INLINE__ void terminate()
  {
    // Nothing to do here. Just wait for the nOS-V worker thread to finalize on its own
  }

  /**
   * Internal implementation of the await function
   */
  __INLINE__ void await()
  {
    // Get the execution state metadata
    auto metadata = (ExecutionState::taskMetadata_t *)getTaskMetadata(_executionState->_executionStateTask);

    // Assertion to check that only the PU task is getting to this
    if (metadata->mainLoop == false) HICR_THROW_RUNTIME("Abort, only PU from the worker mainLoop should get here.\n");

    // Busy wait until the function call fully executed (i.e. the worker mainLoop finished)
    while (_executionState->checkFinalization() == false);
  }

  /**
   * shared instance of the compute resource
   */
  std::shared_ptr<HiCR::backend::hwloc::L0::ComputeResource> _computeResource;

  /**
   * instance of the execution state
   */
  std::unique_ptr<HiCR::backend::nosv::L0::ExecutionState> _executionState;

  /**
   * The affinity struct of nosv
   */
  nosv_affinity_t _nosv_affinity;
};

} // namespace HiCR::backend::nosv::L0
