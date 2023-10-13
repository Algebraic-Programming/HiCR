/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file function.hpp
 * @brief Implements the execution unit (function) class for the shared memory backend.
 * @author S. M. Martin
 * @date 9/10/2023
 */

#pragma once

#include <functional>
#include <hicr/common/coroutine.hpp>
#include <hicr/executionState.hpp>
#include <hicr/backends/sharedMemory/executionUnit.hpp>

namespace HiCR
{

namespace backend
{

namespace sharedMemory
{

class ExecutionState final : public HiCR::ExecutionState
{
 public:

 ExecutionState(const HiCR::ExecutionUnit* executionUnit) : HiCR::ExecutionState(executionUnit)
 {
  // Getting up-casted pointer for the execution unit
  auto e = dynamic_cast<const ExecutionUnit*>(executionUnit);

  // Checking whether the execution unit passed is compatible with this backend
  if (e == NULL) HICR_THROW_LOGIC("The passed execution of type '%s' is not supported by this backend\n", executionUnit->getType());

  // Getting function to execution from the execution unit
  const auto& fc = e->getFunction();

  // Starting coroutine containing the function
  _coroutine.start(fc);
 };

 ExecutionState() = delete;
 ~ExecutionState() = default;

 __USED__ inline void resume() override
  {
   _coroutine.resume();
  }

  __USED__ inline void yield() override
  {
   _coroutine.yield();
  }


 private:

 /**
  *  Task context preserved as a coroutine
  */
 common::Coroutine _coroutine;
};

} // end namespace sharedMemory

} // namespace backend

} // namespace HiCR
