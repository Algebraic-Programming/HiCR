/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file executionState.hpp
 * @brief This file implements the execution state class for the Boost backend
 * @author S. M. Martin
 * @date 9/10/2023
 */

#pragma once

#include <functional>
#include <hicr/core/definitions.hpp>
#include <hicr/core/L0/executionState.hpp>
#include <hicr/backends/boost/coroutine.hpp>
#include <hicr/backends/boost/L0/executionUnit.hpp>

namespace HiCR::backend::boost::L0
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
   * \param[in] argument Argument (closure) to pass to the function to be ran
   */
  __INLINE__ ExecutionState(const std::shared_ptr<HiCR::L0::ExecutionUnit> &executionUnit, void *const argument = nullptr)
    : HiCR::L0::ExecutionState(executionUnit)
  {
    // Getting up-casted pointer for the execution unit
    auto e = static_cast<boost::L0::ExecutionUnit *>(executionUnit.get());

    // Getting function to execution from the execution unit
    const auto &fc = e->getFunction();

    // Starting coroutine containing the function
    _coroutine.start(fc, argument);
  }

  protected:

  __INLINE__ void resumeImpl() override { _coroutine.resume(); }

  __INLINE__ void suspendImpl() override { _coroutine.yield(); }

  __INLINE__ bool checkFinalizationImpl() override { return _coroutine.hasFinished(); }

  private:

  /**
   *  Task context preserved as a coroutine
   */
  boost::Coroutine _coroutine;
};

} // namespace HiCR::backend::boost::L0
