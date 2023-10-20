/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file thread.hpp
 * @brief Implements the processing unit class for the Ascend device backend.
 * @author S. M. Martin & L. Terracciano
 * @date 11/10/2023
 */

#pragma once

#include <hicr/backends/ascend/kernel.hpp>
#include <hicr/common/coroutine.hpp>
#include <hicr/common/exceptions.hpp>
#include <hicr/processingUnit.hpp>

namespace HiCR
{

namespace backend
{

namespace ascend
{

/**
 * Implementation of a kernel-level thread as processing unit for the shared memory backend.
 *
 * This implementation uses PThreads as backend for the creation and management of OS threads..
 */
class Device final : public ProcessingUnit
{
  public:

  __USED__ inline Device(computeResourceId_t device) : ProcessingUnit(device){};

  private:

  __USED__ inline void initializeImpl() override
  {
    HICR_THROW_LOGIC("This function has not been implemented yet");
  }

  __USED__ inline void suspendImpl() override
  {
    HICR_THROW_LOGIC("This function has not been implemented yet");
  }

  __USED__ inline void resumeImpl() override
  {
    HICR_THROW_LOGIC("This function has not been implemented yet");
  }

  __USED__ inline void startImpl(processingUnitFc_t fc) override
  {
    HICR_THROW_LOGIC("This function has not been implemented yet");
  }

  __USED__ inline void startImpl(ExecutionUnit *executionUnit)
  {
    HICR_THROW_LOGIC("This function has not been implemented yet");
  }

  __USED__ inline void terminateImpl() override
  {
    HICR_THROW_LOGIC("This function has not been implemented yet");
  }

  __USED__ inline void awaitImpl() override
  {
    HICR_THROW_LOGIC("This function has not been implemented yet");
  }
};

} // end namespace ascend

} // namespace backend

} // namespace HiCR
