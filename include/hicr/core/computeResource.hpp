/*
 *   Copyright 2025 Huawei Technologies Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file computeResource.hpp
 * @brief Provides a base definition for a HiCR ComputeResource class
 * @author S. M. Martin
 * @date 06/10/2023
 */
#pragma once

#include <string>
#include <nlohmann_json/json.hpp>
#include <hicr/core/definitions.hpp>

namespace HiCR
{

/**
 * This class represents an generic definition for a Compute Resource that:
 *
 * - Represents a single autonomous unit of computing power (e.g., CPU core, device)
 * - This is a copy-able class that only contains metadata
 */
class ComputeResource
{
  public:

  /**
   * Default constructor
   */
  ComputeResource() = default;

  /**
   * Deserializing constructor
   *
   * The instance created will contain all information, if successful in deserializing it, corresponding to the passed processing unit
   * This instance should NOT be used for anything else than reporting/printing the contained resources
   *
   * @param[in] input A JSON-encoded serialized  processing unit information
   */
  ComputeResource(const nlohmann::json &input) { deserialize(input); }

  /**
   * Default destructor
   */
  virtual ~ComputeResource() = default;

  /**
   * Indicates what type of compute unit is contained in this instance
   *
   * \return A string containing a human-readable description of the compute resource type
   */
  [[nodiscard]] __INLINE__ std::string getType() const { return _type; };

  /**
   * Serialization function to enable sharing compute resource information
   *
   * @return JSON-formatted serialized compute resource information
   */
  [[nodiscard]] __INLINE__ nlohmann::json serialize() const
  {
    // Storage for newly created serialized output
    nlohmann::json output;

    // Getting information from the derived class
    serializeImpl(output);

    // Getting compute resource type
    output["Type"] = _type;

    // Returning serialized information
    return output;
  }

  /**
   * De-serialization function to re-construct the serialized compute resource information coming (typically) from remote instances
   *
   * @param[in] input JSON-formatted serialized compute resource information
   */
  __INLINE__ void deserialize(const nlohmann::json &input)
  {
    // Setting compute resource type
    _type = hicr::json::getString(input, "Type");

    // Then call the backend-specific deserialization function
    deserializeImpl(input);
  };

  protected:

  /**
   * Type, used to identify exactly this compute resource's model/technology 
   */
  std::string _type;

  /**
   * Backend-specific implemtation of the serialize function that allows adding more information than the one
   * provided by default by HiCR
   *
   * @param[out] output JSON-formatted serialized compute resource information
   */
  __INLINE__ virtual void serializeImpl(nlohmann::json &output) const {}

  /**
   * Backend-specific implementation of the deserialize function
   *
   * @param[in] input Serialized compute resource information corresponding to the specific backend's topology manager
   */
  __INLINE__ virtual void deserializeImpl(const nlohmann::json &input)
  {
    // Setting device type
    _type = hicr::json::getString(input, "Type");
  }
};

} // namespace HiCR
