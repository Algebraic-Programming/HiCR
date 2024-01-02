/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file computeResource.hpp
 * @brief Provides a base definition for a HiCR ComputeResource class
 * @author S. M. Martin
 * @date 06/10/2023
 */
#pragma once

#include <string>

namespace HiCR
{

namespace L0
{

/**
 * This class represents an abstract definition for a Compute Resource that:
 *
 * - Represents a single autonomous unit of computing power (e.g., CPU core, device)
 * - This is a copy-able class that only contains metadata
 */
class ComputeResource
{
  public:

  /**
   * Indicates what type of compute unit is contained in this instance
   *
   * \return A string containing a human-readable description of the compute resource type
   */
  virtual std::string getType() const = 0;

  /**
   * Default destructor
   */
  virtual ~ComputeResource() = default;

  protected:

  ComputeResource() = default;
};

} // namespace L0

} // namespace HiCR
