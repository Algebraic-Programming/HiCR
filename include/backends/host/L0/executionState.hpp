/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file executionState.hpp
 * @brief This file implements the abstract execution state class for the host (CPU) memory backends
 * @author S. M. Martin
 * @date 9/10/2023
 */

#pragma once

#include <functional>
#include <hicr/definitions.hpp>
#include <hicr/L0/executionState.hpp>
#include <backends/host/coroutine.hpp>
#include <backends/host/L0/executionUnit.hpp>

namespace HiCR
{

namespace backend
{

namespace host
{

namespace L0
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
  __USED__ inline ExecutionState(const std::shared_ptr<HiCR::L0::ExecutionUnit> executionUnit)
    : HiCR::L0::ExecutionState(executionUnit)
  {
    // Getting up-casted pointer for the execution unit
    auto e = dynamic_pointer_cast<host::L0::ExecutionUnit>(executionUnit);

    // Checking whether the execution unit passed is compatible with this backend
    if (e == NULL) HICR_THROW_LOGIC("The passed execution of type '%s' is not supported by this backend\n", executionUnit->getType());

    // Getting function to execution from the execution unit
    const auto &fc = e->getFunction();

    // Starting coroutine containing the function
    _coroutine.start(fc);
  }

  protected:

  __USED__ inline void resumeImpl() override { _coroutine.resume(); }

  __USED__ inline void suspendImpl() { _coroutine.yield(); }

  __USED__ inline bool checkFinalizationImpl() override { return _coroutine.hasFinished(); }

  private:

  /**
   *  Task context preserved as a coroutine
   */
  host::Coroutine _coroutine;
};

} // namespace L0

} // namespace host

} // namespace backend

} // namespace HiCR
