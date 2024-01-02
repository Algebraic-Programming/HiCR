/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file deviceManager.hpp
 * @brief This file implements support for device management of SMP systems
 * @author S. M. Martin
 * @date 18/12/2023
 */

#pragma once

#include "hwloc.h"
#include <backends/sharedMemory/L0/device.hpp>
#include <backends/sharedMemory/L0/computeResource.hpp>
#include <backends/sharedMemory/L0/memorySpace.hpp>
#include <hicr/L1/deviceManager.hpp>

namespace HiCR
{

namespace backend
{

namespace sharedMemory
{

namespace L1
{

/**
 * Implementation of the device manager for SMP systems.
 */
class DeviceManager final : public HiCR::L1::DeviceManager
{
  public:

  /**
   * The constructor is employed to reserve memory required for hwloc
   *
   * \param[in] topology An HWloc topology object that can be used to query the available computational resources
   */
  DeviceManager(hwloc_topology_t *topology) : HiCR::L1::DeviceManager(), _topology(topology) {}

  /**
   * The constructor is employed to free memory required for hwloc
   */
  ~DeviceManager() = default;

  protected:

  __USED__ inline deviceList_t queryDevicesImpl()
  {
    // Disable filters in order to detect instr. caches
    hwloc_topology_set_icache_types_filter(*_topology, HWLOC_TYPE_FILTER_KEEP_ALL);

    // Loading topology
    hwloc_topology_load(*_topology);

    // Creating a single new device representing an SMP system (multicore + shared RAM)
    auto hostDevice = new sharedMemory::L0::Device(queryComputeResources(), queryMemorySpaces());

    // Returning single device
    return {hostDevice};
  }

  private:

  /**
   * Hwloc implementation of the queryComputeResources() function. This will add one compute resource object per HW Thread / Processing Unit (PU) found
   */
  __USED__ inline HiCR::L0::Device::computeResourceList_t queryComputeResources()
  {
    // New compute resource list to return
    HiCR::L0::Device::computeResourceList_t computeResourceList;

    // Creating compute resource list, based on the  processing units (hyperthreads) observed by HWLoc
    std::vector<int> logicalProcessorIds;
    L0::ComputeResource::detectThreadPUs(*_topology, hwloc_get_root_obj(*_topology), 0, logicalProcessorIds);

    for (const auto id : logicalProcessorIds)
    {
      // Creating new compute resource class (of CPU core/processor type)
      auto processor = new L0::ComputeResource(*_topology, id);

      // Adding new resource to the list
      computeResourceList.insert(processor);
    }

    // Returning new compute resource list
    return computeResourceList;
  }

  /**
   * Hwloc implementation of the Backend queryMemorySpaces() function. This will add one memory space object per NUMA domain found
   */
  __USED__ inline HiCR::L0::Device::memorySpaceList_t queryMemorySpaces()
  {
    // New memory space list to return
    HiCR::L0::Device::memorySpaceList_t memorySpaceList;

    // Ask hwloc about number of NUMA nodes and add as many memory spaces as NUMA domains
    auto n = hwloc_get_nbobjs_by_type(*_topology, HWLOC_OBJ_NUMANODE);
    for (int i = 0; i < n; i++)
    {
      // Getting HWLoc object related to this NUMA domain
      auto hwlocObj = hwloc_get_obj_by_type(*_topology, HWLOC_OBJ_NUMANODE, i);

      // Checking whther bound memory allocation and freeing is supported
      auto bindingSupport = L0::LocalMemorySlot::binding_type::strict_non_binding;
      size_t size = 1024;
      auto ptr = hwloc_alloc_membind(*_topology, size, hwlocObj->nodeset, HWLOC_MEMBIND_DEFAULT, HWLOC_MEMBIND_BYNODESET | HWLOC_MEMBIND_STRICT);
      if (ptr != NULL)
      {
        // Attempting to free with hwloc
        auto status = hwloc_free(*_topology, ptr, size);

        // Freeing was successful, then strict binding is supported
        if (status == 0) bindingSupport = L0::LocalMemorySlot::binding_type::strict_binding;
      }

      // Getting memory space size
      auto memSpaceSize = hwlocObj->attr->cache.size;

      // Creating new memory space object
      auto memorySpace = new sharedMemory::L0::MemorySpace(memSpaceSize, hwlocObj, bindingSupport);

      // Storing new memory space
      memorySpaceList.insert(memorySpace);
    }

    // Returning new memory space list
    return memorySpaceList;
  }

  /**
   * Local processor and memory hierarchy topology, as detected by Hwloc
   */
  hwloc_topology_t *const _topology;
};

} // namespace L1

} // namespace sharedMemory

} // namespace backend

} // namespace HiCR
