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
 * @file cache.hpp
 * @brief Defines the Cache class for interacting with the host (CPUs) device type
 * @author O. Korakitis
 * @date 15/11/2023
 */
#pragma once

#include <utility>
#include <vector>
#include <string>
#include <hicr/core/definitions.hpp>
#include <hicr/core/exceptions.hpp>
#include <hicr/core/computeResource.hpp>

namespace HiCR::backend::hwloc
{

/**
 * Object class representing a cache found in a CPU/Processing Unit
 */
class Cache
{
  public:

  /**
   * Type definition for a cache level (L1, L2, L3...)
   */
  enum cacheLevel_t
  {
    /// Cache Level L1
    L1 = 1,

    /// Cache Level L2
    L2 = 2,

    /// Cache Level L3
    L3 = 3,

    /// Cache Level L4
    L4 = 4,

    /// Cache Level L5
    L5 = 5
  };

  /**
   * Constructor for the cache class
   *
   * @param[in] level The level (L1, L1, L2, L3...) detected for this cache instance
   * @param[in] type The type (instruction, data, unified) for this cache
   * @param[in] lineSize The size of each cache line contained within
   * @param[in] shared Indicates whether this cache is shared among others
   * @param[in] size The size of the detected cache
   */
  Cache(const cacheLevel_t level, std::string type, const size_t size, const size_t lineSize, const bool shared)
    : _level(level),
      _type(std::move(type)),
      _cacheSize(size),
      _lineSize(lineSize),
      _shared(shared)
  {}

  /**
   * Deserializing constructor
   *
   * The instance created will contain all information, if successful in deserializing it, corresponding to the passed cache
   * This instance should NOT be used for anything else than reporting/printing the contained resources
   *
   * @param[in] input A JSON-encoded serialized cache information
   */
  Cache(const nlohmann::json &input) { deserialize(input); }

  /**
   * Obtain the size of the cache object
   *
   * \return The cache size in Bytes
   */
  [[nodiscard]] __INLINE__ size_t getSize() const { return _cacheSize; }

  /**
   * Obtain the line size of the cache object
   *
   * \return The cache line size in Bytes
   */
  [[nodiscard]] __INLINE__ size_t getLineSize() const { return _lineSize; }

  /**
   * Obtain the type of the cache object
   *
   * \return The cache type in as a cacheType_t enum value
   */
  [[nodiscard]] __INLINE__ cacheLevel_t getLevel() const { return _level; }

  /**
   * Indicates whether the cache is shared among other procesing units
   *
   * \return True, if the cache is shared; false, otherwise
   */
  [[nodiscard]] __INLINE__ bool getShared() const { return _shared; }

  /**
   * Returns the cache type
   *
   * \return The cache type (instruction, data, unified)
   */
  [[nodiscard]] __INLINE__ const std::string &getType() const { return _type; }

  /**
   * Serialization function to enable sharing cache information
   *
   * @return JSON-formatted serialized cache information
   */
  [[nodiscard]] __INLINE__ nlohmann::json serialize() const
  {
    // Storage for newly created serialized output
    nlohmann::json output;

    // Getting Cache information
    output["Size (Bytes)"]      = getSize();
    output["Line Size (Bytes)"] = getLineSize();
    output["Level"]             = getLevel();
    output["Type"]              = getType();
    output["Shared"]            = getShared();

    // Returning serialized information
    return output;
  }

  /**
   * De-serialization function to obtain the cache values from a serialized JSON object
   *
   * @param[in] input JSON-formatted serialized cache information
   */
  __INLINE__ void deserialize(const nlohmann::json &input)
  {
    std::string key = "Size (Bytes)";
    if (input.contains(key) == false) HICR_THROW_LOGIC("The serialized object contains no '%s' key", key.c_str());
    if (input[key].is_number_unsigned() == false) HICR_THROW_LOGIC("The '%s' entry is not a number", key.c_str());
    _cacheSize = input[key].get<size_t>();

    key = "Line Size (Bytes)";
    if (input.contains(key) == false) HICR_THROW_LOGIC("The serialized object contains no '%s' key", key.c_str());
    if (input[key].is_number_unsigned() == false) HICR_THROW_LOGIC("The '%s' entry is not a number", key.c_str());
    _lineSize = input[key].get<size_t>();

    key = "Level";
    if (input.contains(key) == false) HICR_THROW_LOGIC("The serialized object contains no '%s' key", key.c_str());
    if (input[key].is_number() == false) HICR_THROW_LOGIC("The '%s' entry is not a number. Value: '%s'", key.c_str(), input[key].dump().c_str());
    _level = input[key].get<cacheLevel_t>();

    key = "Type";
    if (input.contains(key) == false) HICR_THROW_LOGIC("The serialized object contains no '%s' key", key.c_str());
    if (input[key].is_string() == false) HICR_THROW_LOGIC("The '%s' entry is not a number", key.c_str());
    _type = input[key].get<std::string>();

    key = "Shared";
    if (input.contains(key) == false) HICR_THROW_LOGIC("The serialized object contains no '%s' key", key.c_str());
    if (input[key].is_boolean() == false) HICR_THROW_LOGIC("The '%s' entry is not a number", key.c_str());
    _shared = input[key].get<bool>();
  }

  private:

  /**
   * Cache level
   */
  cacheLevel_t _level{};

  /**
   * Type of cache (Instruction, Data, Unified)
   */
  std::string _type;

  /**
   * Size of the Cache, in Bytes
   */
  size_t _cacheSize{};

  /**
   * Size of the Cache Line, in Bytes
   */
  size_t _lineSize{};

  /**
   * Flag to indicate whether the flag is of exclusive core use or shared among others
   */
  bool _shared{};

}; // class Cache

} // namespace HiCR::backend::hwloc
