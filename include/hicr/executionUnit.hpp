/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file executionUnit.hpp
 * @brief Provides a base definition for a HiCR ExecutionUnit class
 * @author S. M. Martin
 * @date 06/10/2023
 */
#pragma once

namespace HiCR
{

/**
 * This class represents an abstract definition for a Compute Unit that:
 *
 * - Represents a single state-less computational operation (e.g., a function or a kernel) that can be replicated many times
 * - Is executed in the context of an ExecutionState class
 */
class ExecutionUnit
{
  public:

  /**
   * Defines the default function type to be accepted by most compute managers, when possible.
   */
  typedef std::function<void()> function_t;

  /**
   * Indicates what type of execution unit is contained in this instance
   *
   * \return A string containing a human-readable description of the execution unit type
   */
  virtual std::string getType() const = 0;

  /**
   * Default destructor
   */
  virtual ~ExecutionUnit() = default;

  protected:

  ExecutionUnit() = default;
};

} // namespace HiCR
