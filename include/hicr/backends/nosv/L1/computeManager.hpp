/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file computeManager.hpp
 * @brief This file implements the nOS-V compute manager
 * @author N. Baumann
 * @date 24/02/2025
 */

#pragma once

#include <memory>
#include <hicr/core/definitions.hpp>
#include <hicr/core/exceptions.hpp>
#include <hicr/core/L1/computeManager.hpp>
#include <hicr/backends/nosv/L0/executionUnit.hpp>
#include <hicr/backends/nosv/L0/executionState.hpp>
#include <hicr/backends/nosv/L0/processingUnit.hpp>
#include <hicr/backends/nosv/common.hpp>

namespace HiCR::backend::nosv::L1
{

/**
  * This class represents an abstract definition of a compute manager. That is, the set of functions to be implemented by a given backend that allows
  * the discovery of compute resources, the definition of replicable execution units (functions or kernels to run), and the instantiation of
  * execution states, that represent the execution lifetime of an execution unit.
  */
class ComputeManager : public HiCR::L1::ComputeManager
{
  public:

  /** This function enables the creation of an execution unit.
    *
    * Its default constructor takes a simple replicable CPU-executable function
    *
    * \param[in] executionUnit The replicable function to execute
    * @return The newly created execution unit
    */
  __INLINE__ static std::shared_ptr<HiCR::L0::ExecutionUnit> createExecutionUnit(const std::function<void(void *)> &executionUnit)
  {
    return std::make_shared<HiCR::backend::nosv::L0::ExecutionUnit>(executionUnit);
  }

  /**
    * Creates a new processing unit from the provided compute resource
    *
    * \param[in] computeResource This is the identifier of the compute resource to use to instantiate into a processing unit. The identifier should be one of those provided by the backend. Providing an arbitrary identifier may lead to unexpected behavior.
    *
    * @return A unique pointer to the newly created processing unit. It is important to preserve the uniqueness of this object, since it represents a physical resource (e.g., core) and we do not want to assign it to multiple workers.
    */
  [[nodiscard]] __INLINE__ std::unique_ptr<HiCR::L0::ProcessingUnit> createProcessingUnit(std::shared_ptr<HiCR::L0::ComputeResource> computeResource) const override
  {
    return std::make_unique<HiCR::backend::nosv::L0::ProcessingUnit>(computeResource);
  }

  /**
    * This function enables the creation of an empty execution state object.
    *
    * The instantiation of its internal memory structures is delayed until explicit initialization to reduce memory usage when, for example, scheduling many tasks that do not need to execute at the same time.
    *
    * \param[in] executionUnit The replicable state-less execution unit to instantiate into an execution state
    * \param[in] argument Argument (closure) to pass to the execution unit to make this execution state unique
    * \return A unique pointer to the newly create execution state. It needs to be unique because the state cannot be simultaneously executed my multiple processing units
    */
  __INLINE__ std::unique_ptr<HiCR::L0::ExecutionState> createExecutionState(std::shared_ptr<HiCR::L0::ExecutionUnit> executionUnit, void *const argument = nullptr) const override
  {
    return std::make_unique<HiCR::backend::nosv::L0::ExecutionState>(executionUnit, argument);
  }

  protected:

  /**
    * Backend-specific implementation of the initialize function
    * 
    * @param[in] processingUnit The processing unit to initialize
    */
  __INLINE__ void initializeImpl(std::unique_ptr<HiCR::L0::ProcessingUnit> &processingUnit) override
  {
    auto p = getPUPointer(processingUnit);

    // The logic for starting the posix thread is in the class itself
    p->initialize();
  }

  /**
   * Internal implmentation of the start function
   *
   * @param[in] processingUnit The processing unit to initiate computation with
   * @param[in] executionState The execution state to start running with the given processing unit
   */
  __INLINE__ void startImpl(std::unique_ptr<HiCR::L0::ProcessingUnit> &processingUnit, std::unique_ptr<HiCR::L0::ExecutionState> &executionState) override
  {
    auto p = getPUPointer(processingUnit);

    // The logic for starting the posix thread is in the class itself
    p->start(executionState);
  }

  /**
    * Internal implementation of the suspend function
    * 
    * @param[in] processingUnit The processing unit to suspend
    */
  __INLINE__ void suspendImpl(std::unique_ptr<HiCR::L0::ProcessingUnit> &processingUnit) override
  {
    auto p = getPUPointer(processingUnit);

    // The logic for suspending the posix thread is in the class itself
    p->suspend();
  }

  /**
    * Internal implementation of the resume function
    * 
    * @param[in] processingUnit The processing unit to resume
    */
  __INLINE__ void resumeImpl(std::unique_ptr<HiCR::L0::ProcessingUnit> &processingUnit) override
  {
    auto p = getPUPointer(processingUnit);

    // The logic for resuming the posix thread is in the class itself
    p->resume();
  }

  /**
    * Internal implementation of the terminate function
    * 
    * @param[in] processingUnit The processing unit to terminate
    */
  __INLINE__ void terminateImpl(std::unique_ptr<HiCR::L0::ProcessingUnit> &processingUnit) override
  {
    auto p = getPUPointer(processingUnit);

    // The logic for resuming the posix thread is in the class itself
    p->terminate();
  }

  /**
    * Internal implementation of the await function
    * 
    * @param[in] processingUnit The processing to wait for
    */
  __INLINE__ void awaitImpl(std::unique_ptr<HiCR::L0::ProcessingUnit> &processingUnit) override
  {
    auto p = getPUPointer(processingUnit);

    // The logic for awaiting the posix thread is in the class itself
    p->await();
  }

  /**
    * Getting the pointer of the casted nOS-V PU one
    * 
    * @param[in] processingUnit The processing unit casted into the nosv one
    * \return the dynamicly casted pointer of the processingUnit
    */
  [[nodiscard]] __INLINE__ nosv::L0::ProcessingUnit *getPUPointer(std::unique_ptr<HiCR::L0::ProcessingUnit> &processingUnit)
  {
    // We can only handle processing units of Posix Thread type. Make sure we got the correct one
    // To make it fast and avoid string comparisons, we use the dynamic cast method
    auto p = dynamic_cast<nosv::L0::ProcessingUnit *>(processingUnit.get());

    // If the processing unit is not recognized, throw error. We can use the processing unit's type (string) now.
    if (p == nullptr) HICR_THROW_LOGIC("This compute manager cannot handle processing units of type '%s'", processingUnit->getType());

    // Returning converted pointer
    return p;
  }
};
} // namespace HiCR::backend::nosv::L1
