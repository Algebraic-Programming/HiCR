/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file executionState.hpp
 * @brief This file implements the abstract execution state class for the pthreads backend
 * @author S. M. Martin
 * @date 9/10/2023
 */

#pragma once

#include <functional>
#include <hicr/core/definitions.hpp>
#include <hicr/core/L0/executionState.hpp>
#include <hicr/backends/pthreads/L0/executionUnit.hpp>

namespace HiCR::backend::pthreads::L0
{

/**
 * This class represents the execution state of a resumable function for the pthreads backends.
 */
class ExecutionState final : public HiCR::L0::ExecutionState
{
  public:

  /**
   * Creates a new execution state to be executed in a pthread
   * \param[in] executionUnit The replicable stateless execution unit to instantiate
   * \param[in] argument Argument (closure) to pass to the function to be ran
   */
  __INLINE__ ExecutionState(const std::shared_ptr<HiCR::L0::ExecutionUnit> &executionUnit, void *const argument = nullptr)
    : HiCR::L0::ExecutionState(executionUnit),
      _argument(argument)
  {
    // Getting up-casted pointer for the execution unit
    auto e = static_pointer_cast<pthreads::L0::ExecutionUnit>(executionUnit);

    // Checking whether the execution unit passed is compatible with this backend
    if (e == nullptr) HICR_THROW_LOGIC("The passed execution of type '%s' is not supported by this backend\n", executionUnit->getType().c_str());

    // Getting function to execution from the execution unit
    _fc = e->getFunction();
  }

  protected:

  __INLINE__ void resumeImpl() override
  {
    // Starting the function, passing its argument. It runs to completion.
    _fc(_argument);

    // Setting as finished
    _hasFinished = true;
  }

  __INLINE__ void suspendImpl() override { HICR_THROW_LOGIC("Pthreads execution states do not support the 'suspend' operation"); }

  __INLINE__ bool checkFinalizationImpl() override { return _hasFinished; }

  private:

  /**
   * Function to execute
  */
  ExecutionUnit::pthreadFc_t _fc;

  /**
   * Function arguments
  */
  void *const _argument;

  /**
   * Boolean to check whether the function has finished the execution
  */
  bool _hasFinished = false;
};

} // namespace HiCR::backend::pthreads::L0
