/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file thread.hpp
 * @brief Implements the compute unit (funciton) class for the sequential backend.
 * @author S. M. Martin
 * @date 9/10/2023
 */

#pragma once

#include <functional>
#include <hicr/computeUnit.hpp>

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
typedef std::function<void(void *)> sequentialFc_t;

/**
 * Implementation of a kernel-level thread as processing unit for the shared memory backend.
 *
 * This implementation uses PThreads as backend for the creation and management of OS threads..
 */
class Function final : public ComputeUnit
{
  public:

  /**
   * Constructor for the Process class
   *
   * \param process An id for the process (should be zero)
   */
  __USED__ inline Function(sequentialFc_t fc) : ComputeUnit(), _fc(fc) {};

  __USED__ inline std::string identifyComputeUnitType() override { return "Simple C++ Function"; }
  __USED__ inline const sequentialFc_t& getFunction() { return _fc; }

  private:

  const sequentialFc_t _fc;
};

} // end namespace sequential

} // namespace backend

} // namespace HiCR
