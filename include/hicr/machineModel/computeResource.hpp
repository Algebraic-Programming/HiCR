/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file computeResource.hpp
 * @brief Defines the ComputeResource object, to
 * be used in the Device Model.
 * @author O. Korakitis
 * @date 27/10/2023
 */
#pragma once

#include <map>

#include <hicr/backends/computeManager.hpp>
#include <hicr/backends/memoryManager.hpp>

namespace HiCR
{

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

} // namespace HiCR

