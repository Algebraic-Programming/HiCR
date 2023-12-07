/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file executionState.hpp
 * @brief This file implements the execution state class for the sequential backend
 * @author S. M. Martin
 * @date 9/10/2023
 */

#pragma once

#include <functional>
#include <hicr/backends/sequential/executionUnit.hpp>
#include <hicr/common/coroutine.hpp>
#include <hicr/L0/executionState.hpp>

namespace HiCR
{

namespace backend
{

namespace sequential
{

/**
 * This class represents the execution state of a resumable function for the sequential (and shared memory) backends.
 * It uses a coroutine object to enable suspend/resume functionality.
 */
class ExecutionState final : public HiCR::L0::ExecutionState
{
  public:

  /**
   * Creates a new suspendable execution state (coroutine) for execution based on a sequential execution unit
   * \param[in] executionUnit The replicable stateless execution unit to instantiate
   */
  __USED__ inline ExecutionState(const HiCR::L0::ExecutionUnit *executionUnit) : HiCR::L0::ExecutionState(executionUnit)
  {
    // Getting up-casted pointer for the execution unit
    auto e = dynamic_cast<const sequential::ExecutionUnit *>(executionUnit);

    // Checking whether the execution unit passed is compatible with this backend
    if (e == NULL) HICR_THROW_LOGIC("The passed execution of type '%s' is not supported by this backend\n", executionUnit->getType());

    // Getting function to execution from the execution unit
    const auto &fc = e->getFunction();

    // Starting coroutine containing the function
    _coroutine.start(fc);
  }

  protected:

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
