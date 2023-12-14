/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file computeManager.hpp
 * @brief This is a minimal backend for compute management of (multi-threaded) shared memory systems
 * @authors S. M. Martin, O. Korakitis
 * @date 10/12/2023
 */

#pragma once

#include "hwloc.h"
#include <backends/sequential/L0/executionUnit.hpp>
#include <backends/sharedMemory/L0/computeResource.hpp>
#include <backends/sharedMemory/L0/processingUnit.hpp>
#include <hicr/L1/computeManager.hpp>
#include <memory>

namespace HiCR
{

namespace backend
{

namespace sharedMemory
{

namespace L1
{

/**
 * Implementation of the HWloc-based HiCR Shared Memory backend's compute manager.
 *
 * It detects and returns the processing units detected by the HWLoc library
 */
class ComputeManager final : public HiCR::L1::ComputeManager
{
  public:

  /**
   * Constructor for the compute manager class for the shared memory backend
   *
   * \param[in] topology An HWloc topology object that can be used to query the available computational resources
   */
  ComputeManager(hwloc_topology_t *topology) : HiCR::L1::ComputeManager(), _topology{topology} {}

  /**
   * The constructor is employed to free memory required for hwloc
   */
  ~ComputeManager() = default;

  __USED__ inline HiCR::L0::ExecutionUnit *createExecutionUnit(sequential::L0::ExecutionUnit::function_t executionUnit) override
  {
    return new sequential::L0::ExecutionUnit(executionUnit);
  }

  /**
   * Uses HWloc to discover the sibling logical processors associated with a given logical processor ID
   *
   * \param[in] cpuId The ID of the processor we are doing the search for
   * \returns A vector of processor IDs, siblings of cpuId (expected to have up to 1 in most archs)
   */
  __USED__ static inline std::vector<L0::ComputeResource::logicalProcessorId_t> getCPUSiblings(hwloc_topology_t topology, L0::ComputeResource::logicalProcessorId_t logicalProcessorId)
  {
    hwloc_obj_t pu = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, logicalProcessorId);
    hwloc_obj_t obj = pu;
    if (!obj) HICR_THROW_RUNTIME("Attempting to access a compute resource that does not exist (%lu) in this backend", logicalProcessorId);

    std::vector<L0::ComputeResource::logicalProcessorId_t> ret;

    // Probe if there are *next* siblings
    while (obj->next_sibling)
    {
      ret.push_back(obj->next_sibling->logical_index);
      obj = obj->next_sibling;
    }

    // Return to initial PU object
    obj = pu;
    // Probe if there are *previous* siblings
    while (obj->prev_sibling)
    {
      ret.push_back(obj->prev_sibling->logical_index);
      obj = obj->prev_sibling;
    }

    return ret;
  }

  private:

  /**
   * Pthread implementation of the Backend queryComputeResource() function. This will add one compute resource object per HW Thread / Processing Unit (PU) found
   */
  __USED__ inline computeResourceList_t queryComputeResourcesImpl() override
  {
    // Disable filters in order to detect instr. caches
    hwloc_topology_set_icache_types_filter(*_topology, HWLOC_TYPE_FILTER_KEEP_ALL);

    // Loading topology
    hwloc_topology_load(*_topology);

    // New compute resource list to return
    computeResourceList_t computeResourceList;

    // Creating compute resource list, based on the  processing units (hyperthreads) observed by HWLoc
    std::vector<int> logicalProcessorIds;
    L0::ComputeResource::getThreadPUs(*_topology, hwloc_get_root_obj(*_topology), 0, logicalProcessorIds);

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

  __USED__ inline std::unique_ptr<HiCR::L0::ProcessingUnit> createProcessingUnitImpl(HiCR::L0::ComputeResource* computeResource) const override
  {
    return std::make_unique<L0::ProcessingUnit>(computeResource);
  }

  /**
   * Local processor and memory hierarchy topology, as detected by Hwloc
   */
  hwloc_topology_t *_topology;
};

} // namespace L1

} // namespace sharedMemory

} // namespace backend

} // namespace HiCR
