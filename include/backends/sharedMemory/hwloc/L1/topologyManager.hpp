/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file topologyManager.hpp
 * @brief This file implements support for device management of SMP systems
 * @author S. M. Martin
 * @date 18/12/2023
 */

#pragma once

#include "hwloc.h"
#include <backends/sharedMemory/hwloc/L0/device.hpp>
#include <backends/sharedMemory/hwloc/L0/computeResource.hpp>
#include <backends/sharedMemory/hwloc/L0/memorySpace.hpp>
#include <hicr/L1/topologyManager.hpp>

namespace HiCR
{

namespace backend
{

namespace sharedMemory
{

namespace hwloc
{

namespace L1
{

/**
 * Implementation of the topology manager for shared memory, multicore systems.
 */
class TopologyManager final : public HiCR::L1::TopologyManager
{
  public:

  /**
   * The constructor is employed to reserve memory required for hwloc
   *
   * \param[in] topology An HWloc topology object that can be used to query the available computational resources
   */
  TopologyManager(hwloc_topology_t *topology) : HiCR::L1::TopologyManager(), _topology(topology) {}

  /**
   * The constructor is employed to free memory required for hwloc
   */
  ~TopologyManager() = default;

  protected:

  __USED__ inline deviceList_t queryDevicesImpl()
  {
    // Disable filters in order to detect instr. caches
    hwloc_topology_set_icache_types_filter(*_topology, HWLOC_TYPE_FILTER_KEEP_ALL);

    // Loading topology
    hwloc_topology_load(*_topology);

    // Storage for the new device list 
    deviceList_t deviceList;

    // Ask hwloc about number of NUMA nodes and add as many devices as NUMA domains
    auto n = hwloc_get_nbobjs_by_type(*_topology, HWLOC_OBJ_NUMANODE);
    for (int i = 0; i < n; i++)
    {
      // Creating new device for the current NUMA domain
      auto device = std::make_shared<sharedMemory::hwloc::L0::Device>(i, queryComputeResources(i), queryMemorySpaces(i));

      // Inserting new device into the list
      deviceList.insert(device);
    }  

    // Returning device list
    return deviceList;
  }

  private:

  /**
   * Hwloc implementation of the queryComputeResources() function. This will add one compute resource object per HW Thread / Processing Unit (PU) found
   */
  __USED__ inline HiCR::L0::Device::computeResourceList_t queryComputeResources(const sharedMemory::L0::Device::NUMADomainID_t numaDomainId)
  {
    // New compute resource list to return
    HiCR::L0::Device::computeResourceList_t computeResourceList;

    // Creating compute resource list, based on the  processing units (hyperthreads) observed by HWLoc
    std::vector<int> logicalProcessorIds;
    L0::ComputeResource::detectThreadPUs(*_topology, hwloc_get_root_obj(*_topology), 0, logicalProcessorIds);

    // Adding detected PUs as long they belong to this numa domain
    for (const auto id : logicalProcessorIds) if (L0::ComputeResource::getCpuNumaAffinity(*_topology, id) == numaDomainId)
    {
      // Creating new compute resource class (of CPU core/processor type)
      auto processor = std::make_shared<L0::ComputeResource>(*_topology, id);

      // Adding new resource to the list
      computeResourceList.insert(processor);
    }

    // Returning new compute resource list
    return computeResourceList;
  }

  /**
   * Hwloc implementation of the Backend queryMemorySpaces() function. This will add one memory space object per NUMA domain found
   */
  __USED__ inline HiCR::L0::Device::memorySpaceList_t queryMemorySpaces(const sharedMemory::L0::Device::NUMADomainID_t numaDomainId)
  {
    // New memory space list to return
    HiCR::L0::Device::memorySpaceList_t memorySpaceList;

    // Getting HWLoc object related to this NUMA domain
    auto hwlocObj = hwloc_get_obj_by_type(*_topology, HWLOC_OBJ_NUMANODE, numaDomainId);

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
    auto memorySpace = std::make_shared<sharedMemory::hwloc::L0::MemorySpace>(memSpaceSize, hwlocObj, bindingSupport);

    // Storing new memory space
    memorySpaceList.insert(memorySpace);

    // Returning new memory space list
    return memorySpaceList;
  }

  __USED__ inline void deserializeImpl(const nlohmann::json& input) override
  {
    // Iterating over the device list entries in the serialized input
    for (const auto& device : input["Devices"])
    {
      // Getting device type
      const auto type = device["Type"].get<std::string>();

      // Checking whether the type is correct
      if (type != "NUMA Domain") HICR_THROW_LOGIC("The passed device type '%s' is not compatible with this topology manager", type.c_str());   

      // Deserializing new device
      auto deviceObj = std::make_shared<sharedMemory::hwloc::L0::Device>(device);
      
      // Inserting device into the list
      _deviceList.insert(deviceObj);
    }
  }

  /**
   * Local processor and memory hierarchy topology, as detected by Hwloc
   */
  hwloc_topology_t *const _topology;
};

} // namespace L1

} // namespace hwloc

} // namespace sharedMemory

} // namespace backend

} // namespace HiCR
