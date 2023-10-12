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
#include <hicr/executionUnit.hpp>

namespace HiCR
{

namespace backend
{

namespace sharedMemory
{

/**
 * Implementation of a kernel-level thread as processing unit for the shared memory backend.
 *
 * This implementation uses PThreads as backend for the creation and management of OS threads..
 */
class ExecutionUnit final : public HiCR::ExecutionUnit
{
  public:

 /**
  * Defines the type accepted by the coroutine function as execution unit.
  *
  * \internal The question as to whether std::function entails too much overhead needs to evaluated, and perhaps deprecate it in favor of static function references.
  *           For the time being, this seems adequate enough.
  */
 typedef std::function<void()> function_t;

  /**
   * Constructor for the Process class
   *
   * \param process An id for the process (should be zero)
   */
 ExecutionUnit(function_t fc) : HiCR::ExecutionUnit(), _fc (fc) {};
 ExecutionUnit() = delete;
  ~ExecutionUnit() = default;

  __USED__ inline std::string identifyExecutionUnitType() override { return "C++ Function"; }
  __USED__ inline const function_t& getFunction() { return _fc; }

  private:

  const function_t _fc;
};

} // end namespace sequential

} // namespace backend

} // namespace HiCR
