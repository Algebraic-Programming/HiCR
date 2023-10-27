/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file deviceModel.hpp
 * @brief Defines the lower level Device Model
 * @author O. Korakitis
 * @date 05/10/2023
 */
#pragma once

#include <map>

#include <hicr/backends/computeManager.hpp>
#include <hicr/backends/memoryManager.hpp>

namespace HiCR
{

/**
 * Class definition for an addressable Memory Space.
 *
 * A Device object may comprise one or more such Memory Spaces,
 * on which data can be allocated, copied and communicated among
 * different Memory Spaces, provided there is connectivity
 */
class MemorySpace
{
  protected:

    /* Backend-provided unique ID of the MemorySpace */
    backend::MemoryManager::memorySpaceId_t _id;

    std::string _type;

    /* Size in Bytes */
    size_t _size;

    /* Optional */
    size_t _pageSize;

    /* List of associated processing elements */
    backend::ComputeManager::computeResourceList_t _computeResources;

    // Enhance with bandwidth information, latency...
    // It is not exposed, so either read it from a file or run profiling
    // (probably bad idea)
    size_t _bandwidth;
    size_t _latency;

  public:

  /**
   * Disabled default constructor
   */
    MemorySpace() = delete;

    MemorySpace(
            backend::MemoryManager::memorySpaceId_t id,
            std::string type,
            size_t size,
            size_t pageSize = 4096): /* Default page size; Consider using a constant but in a not dangerous way (e.g. define DEFAULT_PAGESIZE could mess up things) */
        _id(id),
        _type(type),
        _size(size),
        _pageSize(pageSize)
    {
    }

    inline backend::MemoryManager::memorySpaceId_t getId() const
    {
        return _id;
    }

    inline std::string getType() const
    {
        return _type;
    }

    inline size_t getSize() const
    {
        return _size;
    }

    /* If supported, obtain the amount of memory currently in use.
     * In conjunction with the total size above, the user may deduce
     * information like, usage%, if a particular allocation will be
     * possible etc.
     */
    inline size_t getUsage() const
    {
        return 0; //TODO
    }

    inline backend::ComputeManager::computeResourceList_t getComputeUnits() const
    {
        return _computeResources;
    }

    /* Register and Allocate operations.
     *
     * Those should wrap the Backend-provided ones, with the correct MemorySpace IDs.
     */
    inline MemorySlot *registerMemorySlot(void *const ptr, const size_t size);

    inline MemorySlot *allocateMemorySlot(const size_t size);

    inline void deregisterMemorySlot(MemorySlot *const memorySlot);

    inline void freeMemorySlot(MemorySlot *const memorySlot);

}; // class MemorySpace

/**
 * Class definition for a Compute Resource.
 *
 * A Device object may comprise one or more such Compute Resources,
 * on which tasks (as single deployable objects, e.g. function or kernel)
 * can be executed.
 */
class ComputeResource
{
  protected:
    /* Backend-provided unique ID of the ComputeResource */
    computeResourceId_t _id;

    /* The device number, or CPU logical ID */
    size_t _index;

    std::string  _type;

    backend::MemoryManager::memorySpaceList_t _memorySpaces;

    ProcessingUnit *_procUnit;

    /* Optional */
    std::map<backend::MemoryManager::memorySpaceId_t, size_t> _numaDistances;

  public:

  /**
   * Disabled default constructor
   */
    ComputeResource() = delete;

    ComputeResource(
            computeResourceId_t id,
            std::string type,
            ProcessingUnit *procUnit
            ):
        _id(id),
        _type(type),
        _procUnit(procUnit)
    {
    }

    virtual ~ComputeResource() /* consider doing this in Device shutdown() */
    {
        delete _procUnit;
    }

    inline computeResourceId_t getId() const
    {
        return _id;
    }

    inline size_t getIndex() const
    {
        return _index;
    }

    inline std::string getType() const
    {
        return _type;
    }

    // TODO: change this to meaningful functionality
    inline ProcessingUnit *getProcessingUnit() const
    {
        return _procUnit;
    }

    inline backend::MemoryManager::memorySpaceList_t getMemorySpaces() const
    {
        return _memorySpaces;
    }

}; // class ComputeResource

/**
 * Abstract class definition of a Device object.
 *
 * A Device, depending on type, may contain one or more
 * Compute Resources, and one or more addressable Memory Spaces
 */
class DeviceModel
{
  protected:

    backend::ComputeManager *_computeMan;
    backend::MemoryManager *_memoryMan;

    /* List of actual processing elements */
    std::map<computeResourceId_t, ComputeResource *> _computeResources;

    /* List of memories/NUMA nodes */
    std::map<backend::MemoryManager::memorySpaceId_t, MemorySpace *> _memorySpaces;

    std::string _type;

  public:

    virtual void initialize()
    {
    }

    virtual ~DeviceModel() = default;

    inline std::string getType() const
    {
        return _type;
    }

    inline size_t getComputeCount() const
    {
      return _computeResources.size();
    }

    inline size_t getNumMemSpaces() const
    {
      return _memorySpaces.size();
    }

    inline std::set<MemorySpace *> getMemorySpaces() const
    {
        std::set<MemorySpace *> ret;
        for (auto it : _memorySpaces)
            ret.insert(it.second);

        return ret;
    }

    inline std::set<ComputeResource *> getComputeResources() const
    {
        std::set<ComputeResource *> ret;
        for (auto it : _computeResources)
            ret.insert(it.second);

        return ret;
    }

    virtual void shutdown()
    {
        for (auto it : _memorySpaces)
            delete it.second;

        for (auto it : _computeResources)
            delete it.second;

        delete _memoryMan;
        delete _computeMan;
    }

}; // class DeviceModel

} // namespace HiCR

