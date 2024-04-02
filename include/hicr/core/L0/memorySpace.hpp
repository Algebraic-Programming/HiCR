/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file memorySpace.hpp
 * @brief Provides a base definition for a HiCR MemorySpace class
 * @author S. M. Martin & O. Korakitis
 * @date 13/12/2023
 */
#pragma once

#include <string>
#include <nlohmann_json/json.hpp>
#include <hicr/core/exceptions.hpp>

namespace HiCR
{

namespace L0
{

/**
 * This class represents an abstract definition for a Memory Space that:
 *
 * - Represents a autonomous unit of byte-addressable memory (e.g., host memory, NUMA domain, device global RAM)
 * - The space is assumed to be contiguous and have a fixed sized determined at construction time
 * - This is a copy-able class that only contains metadata
 *
 * A Device object may comprise one or more such Memory Spaces
 * on which data can be allocated, copied and communicated among
 * different Memory Spaces, provided there is connectivity
 */
class MemorySpace
{
  public:

  /**
   * Indicates what type of memory space is contained in this instance
   *
   * \return A string containing a human-readable description of the memory space type
   */
  virtual std::string getType() const = 0;

  /**
   * Returns the memory space's size
   *
   * \return The memory space's size
   */
  __INLINE__ virtual const size_t getSize() const { return _size; }

  /**
   *  If supported, obtain the amount of memory currently in use.
   * In conjunction with the total size above, the user may deduce
   * information like, usage%, if a particular allocation will be
   * possible etc.
   *
   * \return The current memory usage for this memory space
   */
  __INLINE__ virtual size_t getUsage() const { return _usage; };

  /**
   * Registers an increase in the used memory size of the current memory space, either by allocation or manual registering
   *
   * \param delta How much (in bytes) has the memory usage increased
   */
  __INLINE__ void increaseUsage(const size_t delta)
  {
    if (_usage + delta > _size)
      HICR_THROW_LOGIC("Increasing memory space usage beyond its capacity (current_usage + increase > capacity | %lu + %lu > %lu)\n", _usage, delta, _size);

    _usage += delta;
  }

  /**
   * Registers a decrease in the used memory size of the current memory space, either by freeing or manual deregistering
   *
   * \param delta How much (in bytes) has the memory usage decreased
   */
  __INLINE__ void decreaseUsage(const size_t delta)
  {
    if (delta > _usage) HICR_THROW_LOGIC("Decreasing memory space usage below zero (probably a bug in HiCR) (current_usage - decrease < 0 | %lu - %lu < 0)\n", _usage, delta);

    _usage -= delta;
  }

  /**
   * Default destructor
   */
  virtual ~MemorySpace() = default;

  /**
   * Serialization function to enable sharing memory space information
   *
   * @return JSON-formatted serialized memory space information
   */
  __INLINE__ nlohmann::json serialize() const
  {
    // Storage for newly created serialized output
    nlohmann::json output;

    // Adding backend-specific information
    serializeImpl(output);

    // Getting memory space type
    output["Type"] = getType();

    // Getting size
    output["Size"] = getSize();

    // Getting current usage
    output["Usage"] = getUsage();

    // Returning serialized information
    return output;
  }

  /**
   * Serialization function to enable sharing memory space information
   *
   * @param[in] input JSON-formatted serialized memory space information
   */
  __INLINE__ void deserialize(const nlohmann::json &input)
  {
    // Obtaining backend-specific information
    deserializeImpl(input);

    // Deserializing size
    std::string key = "Size";
    if (input.contains(key) == false) HICR_THROW_LOGIC("The serialized object contains no '%s' key", key.c_str());
    if (input[key].is_number_unsigned() == false) HICR_THROW_LOGIC("The '%s' entry is not a number", key.c_str());
    _size = input[key].get<size_t>();

    // Deserializing usage
    key = "Usage";
    if (input.contains(key) == false) HICR_THROW_LOGIC("The serialized object contains no '%s' key", key.c_str());
    if (input[key].is_number_unsigned() == false) HICR_THROW_LOGIC("The '%s' entry is not a number", key.c_str());
    _usage = input[key].get<size_t>();
  }

  protected:

  /**
   * Constructor for the MemorySpace class
   *
   * \param[in] size The size of the memory space to create
   */
  MemorySpace(const size_t size)
    : _size(size){};

  /**
   * Default constructor for deserialization purposes
   */
  MemorySpace() = default;

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

  /**
   * The memory space size, defined at construction time
   */
  size_t _size;

  /**
   * This variable keeps track of the memory space usage (through allocation and frees)
   */
  size_t _usage = 0;
};

} // namespace L0

} // namespace HiCR
