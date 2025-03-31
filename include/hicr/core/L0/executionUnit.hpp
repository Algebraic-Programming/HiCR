/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file executionUnit.hpp
 * @brief Provides a base definition for a HiCR Execution Unit class
 * @author S. M. Martin
 * @date 06/10/2023
 */
#pragma once

#include <string>

namespace HiCR::L0
{

/**
 * This class represents an abstract definition for a Execution Unit that:
 *
 * - Represents a single state-less computational operation (e.g., a function or a kernel) that can be replicated many times
 * - Is executed in the context of an ExecutionState class
 */
class ExecutionUnit
{
  public:

  /**
   * Indicates what type of execution unit is contained in this instance
   *
   * \return A string containing a human-readable description of the execution unit type
   */
  [[nodiscard]] virtual std::string getType() const = 0;

  /**
   * Default destructor
   */
  virtual ~ExecutionUnit() = default;

  protected:

  ExecutionUnit() = default;
};

} // namespace HiCR::L0
