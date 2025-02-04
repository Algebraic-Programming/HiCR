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
#include <hicr/backends/host/L0/executionUnit.hpp>
#include <hicr/backends/host/L0/computeResource.hpp>
#include <hicr/backends/host/L1/computeManager.hpp>
#include "../L0/processingUnit.hpp"

namespace HiCR::backend::host::pthreads::L1
{

/**
 * Implementation of the pthread-based HiCR Shared Memory backend's compute manager.
 */
class ComputeManager : public HiCR::backend::host::L1::ComputeManager
{
  public:

  ComputeManager()
    : HiCR::backend::host::L1::ComputeManager()
  {}

  /**
   * The constructor is employed to free memory required for hwloc
   */
  ~ComputeManager() override = default;

  [[nodiscard]] __INLINE__ std::unique_ptr<HiCR::L0::ProcessingUnit> createProcessingUnit(std::shared_ptr<HiCR::L0::ComputeResource> computeResource) const override
  {
    return std::make_unique<host::pthreads::L0::ProcessingUnit>(computeResource);
  }

  private:

  __INLINE__ void initializeImpl(std::unique_ptr<HiCR::L0::ProcessingUnit>& processingUnit) override
  {
    auto p = getPosixThreadPointer(processingUnit);

    // Nothing to do for initialization
  }

  __INLINE__ void startImpl(std::unique_ptr<HiCR::L0::ProcessingUnit>& processingUnit, std::unique_ptr<HiCR::L0::ExecutionState>& executionState) override
  {
    auto p = getPosixThreadPointer(processingUnit);

    // The logic for starting the posix thread is in the class itself
    p->start(executionState);
  }

  __INLINE__ void suspendImpl(std::unique_ptr<HiCR::L0::ProcessingUnit>& processingUnit) override
  {
    auto p = getPosixThreadPointer(processingUnit);

    // The logic for suspending the posix thread is in the class itself
    p->suspend();
  }

  __INLINE__ void resume(std::unique_ptr<HiCR::L0::ProcessingUnit> processingUnit) override
  {
    auto p = getPosixThreadPointer(processingUnit);
  }

  __INLINE__ void terminate(std::unique_ptr<HiCR::L0::ProcessingUnit> processingUnit) override
  {
    auto p = getPosixThreadPointer(processingUnit);
  }

  __INLINE__ void await(std::unique_ptr<HiCR::L0::ProcessingUnit> processingUnit) override
  {
    auto p = getPosixThreadPointer(processingUnit);
  }


  [[nodiscard]] __INLINE__ host::pthreads::L0::ProcessingUnit* getPosixThreadPointer(std::unique_ptr<HiCR::L0::ProcessingUnit>& processingUnit)
  {
     // We can only handle processing units of Posix Thread type. Make sure we got the correct one
    // To make it fast and avoid string comparisons, we use the dynamic cast method
    auto p = dynamic_cast<host::pthreads::L0::ProcessingUnit*>(processingUnit.get()); 

    // If the processing unit is not recognized, throw error. We can use the processing unit's type (string) now.
    if (p == nullptr) HICR_THROW_LOGIC("This compute manager cannot handle processing units of type '%s'", processingUnit->getType());

    // Returning converted pointer
    return p;
  }
};

} // namespace HiCR::backend::host::pthreads::L1
