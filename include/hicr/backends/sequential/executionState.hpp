/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file function.hpp
 * @brief Implements the execution unit (function) class for the sequential backend.
 * @author S. M. Martin
 * @date 9/10/2023
 */

#pragma once

#include <functional>
#include <hicr/backends/sequential/executionUnit.hpp>
#include <hicr/common/coroutine.hpp>
#include <hicr/executionState.hpp>

namespace HiCR
{

namespace backend
{

namespace sequential
{

class ExecutionState final : public HiCR::ExecutionState
{
  protected:

  __USED__ inline void initializeImpl(const HiCR::ExecutionUnit *executionUnit) override
  {
    // Getting up-casted pointer for the execution unit
    auto e = dynamic_cast<const ExecutionUnit *>(executionUnit);

    // Checking whether the execution unit passed is compatible with this backend
    if (e == NULL) HICR_THROW_LOGIC("The passed execution of type '%s' is not supported by this backend\n", executionUnit->getType());

    // Getting function to execution from the execution unit
    const auto &fc = e->getFunction();

    // Starting coroutine containing the function
    _coroutine.start(fc);
  }

  __USED__ inline void resumeImpl() override
  {
    _coroutine.resume();
  }

  __USED__ inline void suspendImpl()
  {
    _coroutine.yield();
  }

  __USED__ inline bool checkFinalizationImpl() override
  {
    return _coroutine.hasFinished();
  }

  private:

  /**
   *  Task context preserved as a coroutine
   */
  common::Coroutine _coroutine;
};

} // end namespace sequential

} // namespace backend

} // namespace HiCR
