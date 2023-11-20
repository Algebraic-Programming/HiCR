/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file memorySpace.hpp
 * @brief Defines the MemorySpace object, to
 * be used in the Device Model.
 * @author O. Korakitis
 * @date 27/10/2023
 */
#pragma once

#include <hicr/backends/computeManager.hpp>
#include <hicr/backends/memoryManager.hpp>

namespace HiCR
{

namespace machineModel
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
    // The problem is how to obtain this information.
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

    inline void addComputeResource(computeResourceId_t id)
    {
        _computeResources.insert(id);
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

} // namespace machineModel

} // namespace HiCR

