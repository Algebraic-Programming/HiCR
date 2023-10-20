/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file executionUnit.hpp
 * @brief This file implements the execution unit class for the sequential backend
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

/**
 * This class represents a replicable C++ executable function for the sequential (and shared memory) backends.
 */
class ExecutionUnit final : public HiCR::ExecutionUnit
{
  public:

  /**
   * Constructor for the execution unit class of the sequential backend
   *
   * \param fc A replicable C++ function to run in this execution unit
   */
  ExecutionUnit(function_t fc) : HiCR::ExecutionUnit(), _fc(fc){};
  ExecutionUnit() = delete;

  /**
   * Default destructor
   */
  ~ExecutionUnit() = default;

  __USED__ inline std::string getType() const override { return "C++ Function"; }

  /**
   * Returns the internal function stored inside this execution unit
   *
   * \return The internal function stored inside this execution unit
   */
  __USED__ inline const function_t &getFunction() const { return _fc; }

  private:

  /**
   * Replicable internal C++ function to run in this execution unit
   */
  const function_t _fc;
};

} // namespace sequential

} // namespace backend

} // namespace HiCR
