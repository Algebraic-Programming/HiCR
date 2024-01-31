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

#include <nlohmann_json/json.hpp>
#include <hicr/definitions.hpp>
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

  /**
   * Serialization function to enable sharing compute resource information
   *
   * @return JSON-formatted serialized compute resource information
   */
  __USED__ inline nlohmann::json serialize() const
  {
    // Storage for newly created serialized output
    nlohmann::json output;

    // Getting information from the derived class
    serializeImpl(output);

    // Getting compute resource type
    output["Type"] = getType();

    // Returning serialized information
    return output;
  }

  /**
   * De-serialization function to re-construct the serialized compute resource information coming (typically) from remote instances
   *
   * @param[in] input JSON-formatted serialized compute resource information
   */
  __USED__ inline void deserialize(const nlohmann::json &input)
  {
    // Then call the backend-specific deserialization function
    deserializeImpl(input);
  };

  protected:

  ComputeResource() = default;

  /**
   * Backend-specific implemtation of the serialize function that allows adding more information than the one
   * provided by default by HiCR
   *
   * @param[out] output JSON-formatted serialized compute resource information
   */
  virtual void serializeImpl(nlohmann::json &output) const = 0;

  /**
   * Backend-specific implementation of the deserialize function
   *
   * @param[in] input Serialized compute resource information corresponding to the specific backend's topology manager
   */
  virtual void deserializeImpl(const nlohmann::json &input) = 0;
};

} // namespace L0

} // namespace HiCR
