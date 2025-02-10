/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file computeManager.hpp
 * @brief This file implements the pthread-based compute manager for host (CPU) backends
 * @authors S. M. Martin, O. Korakitis
 * @date 10/12/2023
 */

#pragma once

#include <memory>
#include <hicr/core/exceptions.hpp>
#include <hicr/backends/hwloc/L0/computeResource.hpp>
#include <hicr/backends/pthreads/L0/executionUnit.hpp>
#include <hicr/backends/pthreads/L0/processingUnit.hpp>
#include <hicr/core/L1/computeManager.hpp>

namespace HiCR::backend::pthreads::L1
{

/**
 * Implementation of the pthread-based HiCR Shared Memory backend's compute manager.
 */
class ComputeManager : public HiCR::L1::ComputeManager
{
  public:

  /**
   * The constructor is employed to reserve memory required for hwloc
   */
  ComputeManager()
    : HiCR::L1::ComputeManager()
  {}

  /**
   * The constructor is employed to free memory required for hwloc
   */
  ~ComputeManager() override = default;

  /** This function enables the creation of an execution unit.
   *
   * Its default constructor takes a simple replicable CPU-executable function
   *
   * \param[in] executionUnit The replicable function to execute
   * @return The newly created execution unit
   */
  __INLINE__ static std::shared_ptr<HiCR::L0::ExecutionUnit> createExecutionUnit(const Coroutine::coroutineFc_t &executionUnit)
  {
    return std::make_shared<pthreads::L0::ExecutionUnit>(executionUnit);
  }

  __INLINE__ std::unique_ptr<HiCR::L0::ExecutionState> createExecutionState(std::shared_ptr<HiCR::L0::ExecutionUnit> executionUnit, void *const argument = nullptr) const override
  {
    // Creating and returning new execution state
    return std::make_unique<pthreads::L0::ExecutionState>(executionUnit, argument);
  }

  [[nodiscard]] __INLINE__ std::unique_ptr<HiCR::L0::ProcessingUnit> createProcessingUnit(std::shared_ptr<HiCR::L0::ComputeResource> computeResource) const override
  {
    return std::make_unique<pthreads::L0::ProcessingUnit>(computeResource);
  }

  private:

  __INLINE__ void initializeImpl(std::unique_ptr<HiCR::L0::ProcessingUnit> &processingUnit) override
  {
    auto p = getPosixThreadPointer(processingUnit);

    // The logic for starting the posix thread is in the class itself
    p->initialize();
  }

  __INLINE__ void startImpl(std::unique_ptr<HiCR::L0::ProcessingUnit> &processingUnit, std::unique_ptr<HiCR::L0::ExecutionState> &executionState) override
  {
    auto p = getPosixThreadPointer(processingUnit);

    // The logic for starting the posix thread is in the class itself
    p->start(executionState);
  }

  __INLINE__ void suspendImpl(std::unique_ptr<HiCR::L0::ProcessingUnit> &processingUnit) override
  {
    auto p = getPosixThreadPointer(processingUnit);

    // The logic for suspending the posix thread is in the class itself
    p->suspend();
  }

  __INLINE__ void resumeImpl(std::unique_ptr<HiCR::L0::ProcessingUnit> &processingUnit) override
  {
    auto p = getPosixThreadPointer(processingUnit);

    // The logic for resuming the posix thread is in the class itself
    p->resume();
  }

  __INLINE__ void terminateImpl(std::unique_ptr<HiCR::L0::ProcessingUnit> &processingUnit) override
  {
    auto p = getPosixThreadPointer(processingUnit);

    // The logic for resuming the posix thread is in the class itself
    p->terminate();
  }

  __INLINE__ void awaitImpl(std::unique_ptr<HiCR::L0::ProcessingUnit> &processingUnit) override
  {
    auto p = getPosixThreadPointer(processingUnit);

    // The logic for awaiting the posix thread is in the class itself
    p->await();
  }

  [[nodiscard]] __INLINE__ pthreads::L0::ProcessingUnit *getPosixThreadPointer(std::unique_ptr<HiCR::L0::ProcessingUnit> &processingUnit)
  {
    // We can only handle processing units of Posix Thread type. Make sure we got the correct one
    // To make it fast and avoid string comparisons, we use the dynamic cast method
    auto p = dynamic_cast<pthreads::L0::ProcessingUnit *>(processingUnit.get());

    // If the processing unit is not recognized, throw error. We can use the processing unit's type (string) now.
    if (p == nullptr) HICR_THROW_LOGIC("This compute manager cannot handle processing units of type '%s'", processingUnit->getType());

    // Returning converted pointer
    return p;
  }
};

} // namespace HiCR::backend::pthreads::L1
