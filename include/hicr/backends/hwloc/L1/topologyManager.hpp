/*
 *   Copyright 2025 Huawei Technologies Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file topologyManager.hpp
 * @brief This file implements the topology manager class for the HWLoc-based backend
 * @author S. M. Martin
 * @date 18/12/2023
 */

#pragma once

#include "hwloc.h"
#include <hicr/backends/hwloc/L0/device.hpp>
#include <hicr/backends/hwloc/L0/computeResource.hpp>
#include <hicr/backends/hwloc/L0/memorySpace.hpp>
#include <hicr/core/L0/topology.hpp>
#include <hicr/core/L1/topologyManager.hpp>

namespace HiCR::backend::hwloc::L1
{

/**
 * Max storage for memspace information
 */
constexpr size_t _HWLOC_MAX_MEMSPACE_SIZE = 1024;

/**
 * Implementation of the HWLoc-based topology manager for CPU resource detection
 */
class TopologyManager final : public HiCR::L1::TopologyManager
{
  public:

  /**
   * The constructor is employed to reserve memory required for hwloc
   *
   * \param[in] topology An HWloc topology object that can be used to query the available computational resources
   */
  TopologyManager(hwloc_topology_t *topology)
    : HiCR::L1::TopologyManager(),
      _topology(topology)
  {}

  /**
   * The constructor is employed to free memory required for hwloc
   */
  ~TopologyManager() override = default;

  __INLINE__ HiCR::L0::Topology queryTopology() override
  {
    // Disable filters in order to detect instr. caches
    hwloc_topology_set_icache_types_filter(*_topology, HWLOC_TYPE_FILTER_KEEP_ALL);

    // Loading HWLoc topology object
    hwloc_topology_load(*_topology);

    // Storage for the new HICR topology
    HiCR::L0::Topology t;

    // Ask hwloc about number of NUMA nodes and add as many devices as NUMA domains
    auto n = hwloc_get_nbobjs_by_type(*_topology, HWLOC_OBJ_NUMANODE);
    for (int i = 0; i < n; i++)
    {
      // Creating new device for the current NUMA domain
      auto d = std::make_shared<hwloc::L0::Device>(i, queryComputeResources(i), queryMemorySpaces(i));

      // Inserting new device into the list
      t.addDevice(d);
    }

    // Returning the created topology
    return t;
  }

  /**
   * Static implementation of the _deserializeTopology Function
   * @param[in] topology see: _deserializeTopology
   * @return see: _deserializeTopology
   */
  __INLINE__ static HiCR::L0::Topology deserializeTopology(const nlohmann::json &topology)
  {
    // Verifying input's syntax
    HiCR::L0::Topology::verify(topology);

    // New topology to create
    HiCR::L0::Topology t;

    // Iterating over the device list entries in the serialized input
    for (const auto &device : topology["Devices"])
    {
      // Getting device type
      const auto type = device["Type"].get<std::string>();

      // If the device type is recognized, add it to the new topology
      if (type == "NUMA Domain") t.addDevice(std::make_shared<hwloc::L0::Device>(device));
    }

    // Returning new topology
    return t;
  }

  [[nodiscard]] __INLINE__ HiCR::L0::Topology _deserializeTopology(const nlohmann::json &topology) const override { return deserializeTopology(topology); }

  /**
   * This function represents the default intializer for this backend
   *
   * @return A unique pointer to the newly instantiated backend class
   */
  __INLINE__ static std::unique_ptr<HiCR::L1::TopologyManager> createDefault()
  {
    // Creating HWloc topology object
    auto topology = new hwloc_topology_t;

    // Reserving memory for hwloc
    hwloc_topology_init(topology);

    // Initializing HWLoc-based topology manager
    return std::make_unique<HiCR::backend::hwloc::L1::TopologyManager>(topology);
  }

  private:

  /**
   * Hwloc implementation of the queryComputeResources() function. This will add one compute resource object per HW Thread / Processing Unit (PU) found
   */
  __INLINE__ HiCR::L0::Device::computeResourceList_t queryComputeResources(const hwloc::L0::Device::NUMADomainID_t numaDomainId)
  {
    // New compute resource list to return
    HiCR::L0::Device::computeResourceList_t computeResourceList;

    // Creating compute resource list, based on the  processing units (hyperthreads) observed by HWLoc
    std::vector<HiCR::backend::hwloc::L0::ComputeResource::logicalProcessorId_t> logicalProcessorIds;
    HiCR::backend::hwloc::L0::ComputeResource::detectThreadPUs(*_topology, hwloc_get_root_obj(*_topology), 0, logicalProcessorIds);

    // Adding detected PUs as long they belong to this numa domain
    for (const auto id : logicalProcessorIds)
      if (L0::ComputeResource::getCpuNumaAffinity(*_topology, id) == numaDomainId)
      {
        // Creating new compute resource class (of CPU core/processor type)
        auto processor = std::make_shared<HiCR::backend::hwloc::L0::ComputeResource>(*_topology, id);

        // Adding new resource to the list
        computeResourceList.push_back(processor);
      }

    // Returning new compute resource list
    return computeResourceList;
  }

  /**
   * Hwloc implementation of the Backend queryMemorySpaces() function. This will add one memory space object per NUMA domain found
   */
  __INLINE__ HiCR::L0::Device::memorySpaceList_t queryMemorySpaces(const hwloc::L0::Device::NUMADomainID_t numaDomainId)
  {
    // New memory space list to return
    HiCR::L0::Device::memorySpaceList_t memorySpaceList;

    // Getting HWLoc object related to this NUMA domain
    auto hwlocObj = hwloc_get_obj_by_type(*_topology, HWLOC_OBJ_NUMANODE, numaDomainId);

    // Checking whther bound memory allocation and freeing is supported
    auto   bindingSupport = L0::LocalMemorySlot::binding_type::strict_non_binding;
    size_t size           = _HWLOC_MAX_MEMSPACE_SIZE;
    auto   ptr            = hwloc_alloc_membind(*_topology, size, hwlocObj->nodeset, HWLOC_MEMBIND_DEFAULT, HWLOC_MEMBIND_BYNODESET | HWLOC_MEMBIND_STRICT);
    if (ptr != nullptr)
    {
      // Attempting to free with hwloc
      auto status = hwloc_free(*_topology, ptr, size);

      // Freeing was successful, then strict binding is supported
      if (status == 0) bindingSupport = L0::LocalMemorySlot::binding_type::strict_binding;
    }

    // Getting memory space size
    auto memSpaceSize = hwlocObj->attr->cache.size;

    // Creating new memory space object
    auto memorySpace = std::make_shared<hwloc::L0::MemorySpace>(memSpaceSize, hwlocObj, bindingSupport);

    // Storing new memory space
    memorySpaceList.push_back(memorySpace);

    // Returning new memory space list
    return memorySpaceList;
  }

  /**
   * Local processor and memory hierarchy topology, as detected by Hwloc
   */
  hwloc_topology_t *const _topology;
};

} // namespace HiCR::backend::hwloc::L1
