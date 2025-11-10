/*
 *   Copyright 2025 Huawei Technologies Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
#include <hicr/core/executionState.hpp>
#include <hicr/backends/pthreads/executionUnit.hpp>

namespace HiCR::backend::pthreads
{

/**
 * This class represents the execution state of a resumable function for the pthreads backends.
 */
class ExecutionState final : public HiCR::ExecutionState
{
  public:

  /**
   * Creates a new execution state to be executed in a pthread
   * \param[in] executionUnit The replicable stateless execution unit to instantiate
   * \param[in] argument Argument (closure) to pass to the function to be ran
   */
  __INLINE__ ExecutionState(const std::shared_ptr<HiCR::ExecutionUnit> &executionUnit, void *const argument = nullptr)
    : HiCR::ExecutionState(executionUnit),
      _argument(argument)
  {
    // Getting up-casted pointer for the execution unit
    auto e = std::static_pointer_cast<pthreads::ExecutionUnit>(executionUnit);

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

} // namespace HiCR::backend::pthreads
