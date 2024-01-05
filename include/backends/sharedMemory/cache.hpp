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

namespace sharedMemory
{

/**
 * Object class representing a cache found in a CPU/Processing Unit
 */
class Cache
{
  public:

  typedef unsigned int cacheLevel_t;

  /**
   * Constructor for the cache class
   *
   * @param[in] level The level (L1, L1, L2, L3...) detected for this cache instance
   * @param[in] Type The type (instruction, data, unified) for this cache
   * @param[in] lineSize The size of each cache line contained within
   * @param[in] shared Indicates whether this cache is shared among others
   * @param[in] size The size of the detected cache
   */
  Cache(const cacheLevel_t level, const std::string& type, const size_t size, const size_t lineSize, const bool shared) :
   _level(level),
   _type(type),
   _cacheSize(size),
   _lineSize(lineSize),
   _shared(shared)
  {
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
  __USED__ inline const std::string& getType() const
  {
    return _type;
  }

  /**
   * Serialization function to enable sharing device information
   *
   * @return JSON-formatted serialized device information
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

 private:

  /**
   * Cache level
   */
  const cacheLevel_t _level;

  /**
   * Type of cache (Instruction, Data, Unified)
  */
  const std::string _type;

  /**
   * Size of the Cache, in Bytes
   */
  const size_t _cacheSize;

  /**
   * Size of the Cache Line, in Bytes
   */
  const size_t _lineSize;

  /**
   * Flag to indicate whether the flag is of exclusive core use or shared among others
  */
  const bool _shared;

}; // class Cache

} // namespace sharedMemory

} // namespace backend

} // namespace HiCR
