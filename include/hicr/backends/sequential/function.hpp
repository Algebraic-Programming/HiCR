/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file function.hpp
 * @brief Implements the compute unit (function) class for the sequential backend.
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

namespace sequential
{

/**
 * Defines the type accepted by the coroutine function as execution unit.
 *
 * \internal The question as to whether std::function entails too much overhead needs to evaluated, and perhaps deprecate it in favor of static function references. For the time being, this seems adequate enough.
 *
 */
typedef common::Coroutine::coroutineFc_t sequentialFc_t;

/**
 * Implementation of a kernel-level thread as processing unit for the shared memory backend.
 *
 * This implementation uses PThreads as backend for the creation and management of OS threads..
 */
class Function final : public ExecutionUnit
{
  public:

  /**
   * Constructor for the Process class
   *
   * \param process An id for the process (should be zero)
   */
  Function(sequentialFc_t fc) : ExecutionUnit() {_fc = fc;};
  Function() = delete;
  ~Function() = default;

  __USED__ inline std::string identifyExecutionUnitType() override { return "Simple C++ Function"; }
  __USED__ inline const sequentialFc_t& getFunction() { return _fc; }

  private:

  sequentialFc_t _fc;
};

} // end namespace sequential

} // namespace backend

} // namespace HiCR
