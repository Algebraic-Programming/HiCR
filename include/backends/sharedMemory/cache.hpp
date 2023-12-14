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

  /**
   * Set of cache types commonly encountered
   */
  typedef std::string cacheType_t;

  protected:

  /**
   * Size of the Cache, in Bytes
   */
  size_t _cacheSize;

  /**
   * Size of the Cache Line, in Bytes
   */
  size_t _lineSize;

  /**
   * Type of the Cache object
   */
  cacheType_t _level;

  /**
   * Storage of Compute Units associated with the Cache;
   * If the cache is shared among multiple cores, the multiple IDs
   * will appear here. There is currently redunduncy in the representation,
   * as all cores that share the cache will keep a copy of this information
   */
  std::vector<HiCR::L0::ComputeResource*> _associatedComputeUnits;

  public:

  Cache(const cacheType_t level, const size_t size) : 
    _cacheSize(size),
     _level(level)
     {}

  /**
   * Obtain the size of the cache object
   *
   * \return The cache size in Bytes
   */
  inline size_t getCacheSize() const
  {
    return _cacheSize;
  }

  /**
   * Set the cache size with information obtained from the backend.
   * This should be used only during initialization / resource detection.
   *
   * \param[in] size The size of the cache in Bytes
   */
  inline void setCacheSize(size_t size)
  {
    _cacheSize = size;
  }

  /**
   * Obtain the line size of the cache object
   *
   * \return The cache line size in Bytes
   */
  inline size_t getLineSize() const
  {
    return _lineSize;
  }

  /**
   * Set the cache line size with information obtained from the backend (or 64 Bytes by default).
   * This should be used only during initialization / resource detection.
   *
   * \param[in] lsize The size of the cache line in Bytes
   */
  inline void setLineSize(size_t lsize = 64)
  {
    _lineSize = lsize;
  }

  /**
   * Obtain the type of the cache object
   *
   * \return The cache type in as a cacheType_t enum value
   */
  inline cacheType_t getCacheType() const
  {
    return _level;
  }

  /**
   * Set the cache type with information obtained from the backend.
   * This should be used only during initialization / resource detection.
   *
   * \param[in] t The type of the cache as a cacheType_t enum value
   */
  inline void setCacheType(cacheType_t t)
  {
    _level = t;
  }

  /**
   * Obtain the associated CPU ID(s) of the cache object
   *
   * \return An std::vector of the CPU IDs
   */
  inline std::vector<HiCR::L0::ComputeResource*> getAssociatedComputeUnits() const
  {
    return _associatedComputeUnits;
  }

  /**
   * Set compute resource ID associated with a cache; used for private caches
   * This should be used only during initialization / resource detection.
   *
   * \param[in] id The HiCR::L0::computeResourceId_t ID of the Processing Unit the cache belongs to
   */
  inline void setAssociatedComputeUnit(HiCR::L0::ComputeResource* computeUnit)
  {
    _associatedComputeUnits.resize(1);
    _associatedComputeUnits[0] = computeUnit;
  }

  /**
   * Add compute resource ID associated with a cache; used for shared caches
   * This should be used only during initialization / resource detection.
   *
   * \param[in] id The HiCR::L0::computeResourceId_t ID of a Processing Unit sharing the cache
   */
  inline void addAssociatedComputeUnit(HiCR::L0::ComputeResource* computeUnit)
  {
    _associatedComputeUnits.push_back(computeUnit);
  }

  /**
   * This function checks if a cache object is shared among multiple cores or is private.
   *
   * \return True, if the cache is shared; False if it is private.
   */
  inline bool isShared() const
  {
    return (_associatedComputeUnits.size() > 1);
  }

}; // class Cache

} // namespace sharedMemory

} // namespace backend

} // namespace HiCR
