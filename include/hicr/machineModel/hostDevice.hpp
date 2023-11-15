/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file hostDevice.hpp
 * @brief Defines the API for interacting with the host (CPUs) device type
 * @author O. Korakitis
 * @date 05/10/2023
 */
#pragma once

#include <hicr/machineModel/deviceModel.hpp>
#include <hicr/backends/sharedMemory/computeManager.hpp>
#include <hicr/backends/sharedMemory/memoryManager.hpp>

namespace HiCR
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
    typedef enum
    {
        L1i, /* L1 instruction */
        L1d, /* L1 data */
        L2,
        L3
    } cacheType;

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
    cacheType _level;

    /**
     * Storage of Compute Units associated with the Cache;
     * If the cache is shared among multiple cores, the multiple IDs
     * will appear here. There is currently redunduncy in the representation,
     * as all cores that share the cache will keep a copy of this information
     */
    std::vector<computeResourceId_t> _associatedComputeUnit;

  public:

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
     * \param[in] size The size of the cache line in Bytes
     */
    inline void setLineSize(size_t lsize = 64)
    {
      _lineSize = lsize;
    }

    /**
     * Obtain the type of the cache object
     *
     * \return The cache type in as a cacheType enum value
     */
    inline cacheType getCacheType() const
    {
      return _level;
    }

    /**
     * Set the cache type with information obtained from the backend.
     * This should be used only during initialization / resource detection.
     *
     * \param[in] size The type of the cache as a cacheType enum value
     */
    inline void setCacheType(cacheType t)
    {
      _level = t;
    }

    /**
     * Set compute resource ID associated with a cache; used for private caches
     * This should be used only during initialization / resource detection.
     *
     * \param[in] id The computeResourceId_t ID of the Processing Unit the cache belongs to
     */
    inline void setAssociatedComputeUnit(computeResourceId_t id)
    {
      _associatedComputeUnit.resize(1);
      _associatedComputeUnit[0] = id;
    }

    /**
     * Add compute resource ID associated with a cache; used for shared caches
     * This should be used only during initialization / resource detection.
     *
     * \param[in] id The computeResourceId_t ID of a Processing Unit sharing the cache
     */
    inline void addAssociatedComputeUnit(computeResourceId_t id)
    {
        _associatedComputeUnit.push_back(id);
    }

    /**
     * This function checks if a cache object is shared among multiple cores or is private.
     *
     * \return True, if the cache is shared; False if it is private.
     */
    inline bool isShared() const
    {
      return (_associatedComputeUnit.size() > 1);
    }

}; // class Cache

class CPU: public ComputeResource
{
  private:
    unsigned int _systemId;
    unsigned int _logicalId;
    unsigned int *_siblings; /* Optional */
    std::vector<Cache> _privateCaches;

  public:
    CPU(computeResourceId_t id):
        ComputeResource(id, "Core")
    {
    }


}; // class CPU

class HostDevice final: public DeviceModel
{
  private:
    std::vector<Cache> _sharedCaches; // revisit

  public:
    HostDevice()
    {
        _type = "host";
    }

    void initialize() override
   {
       // Creating HWloc topology object
       hwloc_topology_t topology;

       // Reserving memory for hwloc
       hwloc_topology_init(&topology);

       // Initialize Backend-specific Compute & Memory Managers
       _computeMan = new backend::sharedMemory::ComputeManager(&topology);
       _memoryMan  = new backend::sharedMemory::MemoryManager(&topology);

       _computeMan->queryComputeResources();
       _memoryMan->queryMemorySpaces();

        // Populate our own resource representation based on the backend-specific Managers
        for (auto m : _memoryMan->getMemorySpaceList())
        {
            backend::MemoryManager::memorySpaceId_t tmp_id = m;
            std::string tmp_type = "NUMA Domain";
            MemorySpace *ms = new MemorySpace(
                    tmp_id, /* memorySpaceId_t */
                    tmp_type, /* type */
                    _memoryMan->getMemorySpaceSize(tmp_id) /* size */
                    );
            _memorySpaces.insert(std::make_pair(tmp_id, ms));
        }

        for (auto c : _computeMan->getComputeResourceList())
        {
            computeResourceId_t tmp_id = c;
            CPU *cmp = new CPU(
                    tmp_id /* computeResourceId_t */
                    );
            _computeResources.insert(std::make_pair(tmp_id, cmp));
        }

   }

   void shutdown() override 
    {
        for (auto it : _memorySpaces)
            delete it.second;

        for (auto it : _computeResources)
            delete it.second;

        delete(_computeMan);
        delete(_memoryMan);
    }

}; // class HostDevice


} // namespace HiCR

