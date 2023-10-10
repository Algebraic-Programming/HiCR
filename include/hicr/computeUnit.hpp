/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file computeUnit.hpp
 * @brief Provides a definition for a HiCR ComputeUnit class
 * @author S. M. Martin
 * @date 06/10/2023
 */
#pragma once

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <hicr/common/definitions.hpp>

namespace HiCR
{

/**
 * Type definition for a generic memory slot identifier
 */
typedef boost::uuids::uuid computeUnit_t;

/**
 * This class represents an abstract definition for a Compute Unit that:
 *
 * - Represents a single state-less computational operation (a.k.a. kernel) that can be replicated many times
 */
class ComputeUnit
{
  public:

  virtual computeUnit_t getId() { return _id; }

  virtual std::string identifyComputeUnitType() = 0;

  protected:

  ComputeUnit() : _id(boost::uuids::random_generator()()) {};
  ~ComputeUnit() = default;

  private:

  computeUnit_t _id;
};

} // namespace HiCR
