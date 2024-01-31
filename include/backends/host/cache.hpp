/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file cache.hpp
 * @brief Defines the Cache class for interacting with the host (CPUs) device type
 * @author O. Korakitis
 * @date 15/11/2023
 */
#pragma once

#include <vector>
#include <string>
#include <hicr/L0/computeResource.hpp>

namespace HiCR
{

namespace backend
{

namespace host
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
  typedef unsigned int cacheLevel_t;

  /**
   * Constructor for the cache class
   *
   * @param[in] level The level (L1, L1, L2, L3...) detected for this cache instance
   * @param[in] type The type (instruction, data, unified) for this cache
   * @param[in] lineSize The size of each cache line contained within
   * @param[in] shared Indicates whether this cache is shared among others
   * @param[in] size The size of the detected cache
   */
  Cache(const cacheLevel_t level, const std::string &type, const size_t size, const size_t lineSize, const bool shared) : _level(level),
                                                                                                                          _type(type),
                                                                                                                          _cacheSize(size),
                                                                                                                          _lineSize(lineSize),
                                                                                                                          _shared(shared)
  {
  }

  /**
   * Deserializing constructor
   *
   * The instance created will contain all information, if successful in deserializing it, corresponding to the passed cache
   * This instance should NOT be used for anything else than reporting/printing the contained resources
   *
   * @param[in] input A JSON-encoded serialized cache information
   */
  Cache(const nlohmann::json &input)
  {
    deserialize(input);
  }

  /**
   * Obtain the size of the cache object
   *
   * \return The cache size in Bytes
   */
  __USED__ inline size_t getSize() const
  {
    return _cacheSize;
  }

  /**
   * Obtain the line size of the cache object
   *
   * \return The cache line size in Bytes
   */
  __USED__ inline size_t getLineSize() const
  {
    return _lineSize;
  }

  /**
   * Obtain the type of the cache object
   *
   * \return The cache type in as a cacheType_t enum value
   */
  __USED__ inline cacheLevel_t getLevel() const
  {
    return _level;
  }

  /**
   * Indicates whether the cache is shared among other procesing units
   *
   * \return True, if the cache is shared; false, otherwise
   */
  __USED__ inline bool getShared() const
  {
    return _shared;
  }

  /**
   * Returns the cache type
   *
   * \return The cache type (instruction, data, unified)
   */
  __USED__ inline const std::string &getType() const
  {
    return _type;
  }

  /**
   * Serialization function to enable sharing cache information
   *
   * @return JSON-formatted serialized cache information
   */
  __USED__ inline nlohmann::json serialize() const
  {
    // Storage for newly created serialized output
    nlohmann::json output;

    // Getting Cache information
    output["Size (Bytes)"] = getSize();
    output["Line Size (Bytes)"] = getLineSize();
    output["Level"] = getLevel();
    output["Type"] = getType();
    output["Shared"] = getShared();

    // Returning serialized information
    return output;
  }

  /**
   * De-serialization function to obtain the cache values from a serialized JSON object
   *
   * @param[in] input JSON-formatted serialized cache information
   */
  __USED__ inline void deserialize(const nlohmann::json &input)
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
    if (input[key].is_number_unsigned() == false) HICR_THROW_LOGIC("The '%s' entry is not a number", key.c_str());
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

  protected:

  /**
   * Cache level
   */
  cacheLevel_t _level;

  /**
   * Type of cache (Instruction, Data, Unified)
   */
  std::string _type;

  /**
   * Size of the Cache, in Bytes
   */
  size_t _cacheSize;

  /**
   * Size of the Cache Line, in Bytes
   */
  size_t _lineSize;

  /**
   * Flag to indicate whether the flag is of exclusive core use or shared among others
   */
  bool _shared;

}; // class Cache

} // namespace host

} // namespace backend

} // namespace HiCR
