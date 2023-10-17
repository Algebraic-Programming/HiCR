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
#include <hicr/executionUnit.hpp>

namespace HiCR
{

namespace backend
{

namespace sequential
{

class ExecutionUnit final : public HiCR::ExecutionUnit
{
  public:

  /**
   * Constructor for the Process class
   *
   * \param process An id for the process (should be zero)
   */
 ExecutionUnit(function_t fc) : HiCR::ExecutionUnit(), _fc (fc) {};
 ExecutionUnit() = delete;
  ~ExecutionUnit() = default;

  __USED__ inline std::string getType() const override { return "C++ Function"; }
  __USED__ inline const function_t& getFunction() const { return _fc; }

  private:

  const function_t _fc;
};

} // namespace sequential

} // namespace backend

} // namespace HiCR
