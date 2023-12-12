/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file computeUnit.hpp
 * @brief Provides a base definition for a HiCR ComputeUnit class
 * @author S. M. Martin
 * @date 06/10/2023
 */
#pragma once

#include <functional>
#include <string>

namespace HiCR
{

namespace L0
{

/**
 * This class represents an abstract definition for a Compute Unit that:
 *
 * - Represents a single autonomous unit of computing power (e.g., CPU core, device)
 */
class ComputeUnit
{
  public:

  /**
   * Indicates what type of compute unit is contained in this instance
   *
   * \return A string containing a human-readable description of the compute unit type
   */
  virtual std::string getType() const = 0;

  /**
   * Default destructor
   */
  virtual ~ComputeUnit() = default;

  protected:

  ComputeUnit() = default;
};

} // namespace L0

} // namespace HiCR
