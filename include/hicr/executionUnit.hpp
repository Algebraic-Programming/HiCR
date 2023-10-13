/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file executeUnit.hpp
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
 */
class ExecutionUnit
{
  public:

  virtual std::string getType() const = 0;

  protected:

  ExecutionUnit() = default;
  ~ExecutionUnit() = default;
};

} // namespace HiCR
