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

#include <hicr/machineModel/computeResource.hpp>
#include <hicr/machineModel/memorySpace.hpp>

namespace HiCR
{

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

