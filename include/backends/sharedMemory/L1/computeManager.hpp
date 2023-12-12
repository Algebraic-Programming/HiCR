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
   * Uses HWloc to recursively (tree-like) identify the system's basic processing units (PUs)
   *
   * \param[in] topology An HWLoc topology object, already initialized
   * \param[in] obj The root HWLoc object for the start of the exploration tree at every recursion level
   * \param[in] depth Stores the current exploration depth level, necessary to return only the processing units at the leaf level
   * \param[out] threadPUs Storage for the found procesing units
   */
  __USED__ inline static void getThreadPUs(hwloc_topology_t topology, hwloc_obj_t obj, int depth, std::vector<int> &threadPUs)
  {
    if (obj->arity == 0) threadPUs.push_back(obj->logical_index);
    for (unsigned int i = 0; i < obj->arity; i++) getThreadPUs(topology, obj->children[i], depth + 1, threadPUs);
  }

  /**
   * Uses HWloc to discover the (physical) Core ID, associated with a given logical processor ID
   *
   * \param[in] cpuId The ID of the processor we are doing the search for
   * \returns The ID of the associated memory space
   */
  inline size_t getCpuSystemId(HiCR::L0::ComputeResource* computeResource) const
  {
    // Getting up-casted pointer for the MPI instance
    auto core = dynamic_cast<L0::ComputeResource *const>(computeResource);

    // Checking whether the execution unit passed is compatible with this backend
    if (core == NULL) HICR_THROW_LOGIC("The passed compute unit is not supported by this compute manager\n");

    // Getting core affinity
    auto cpuId = core->getAffinity();

    hwloc_obj_t obj = hwloc_get_obj_by_type(*_topology, HWLOC_OBJ_PU, cpuId);
    if (!obj)  HICR_THROW_RUNTIME( "Attempting to access a compute resource that does not exist (%lu) in this backend", cpuId);

    // Acquire the parent core object
    // There is an asumption here that a HWLOC_OBJ_PU type always has a parent of type HWLOC_OBJ_CORE,
    // which is consistent with current HWloc, but maybe reconsider it.
    obj = obj->parent;
    if (obj->type != HWLOC_OBJ_CORE)  HICR_THROW_RUNTIME("Unexpected hwloc object type while trying to access Core/CPU (%lu)", cpuId);

    return obj->logical_index;
  }

  /**
   * Uses HWloc to discover the sibling logical processors associated with a given logical processor ID
   *
   * \param[in] cpuId The ID of the processor we are doing the search for
   * \returns A vector of processor IDs, siblings of cpuId (expected to have up to 1 in most archs)
   */
  inline std::vector<unsigned> getCpuSiblings(HiCR::L0::ComputeResource* computeResource) const
  {
    // Getting up-casted pointer for the MPI instance
    auto core = dynamic_cast<L0::ComputeResource *const>(computeResource);

    // Checking whether the execution unit passed is compatible with this backend
    if (core == NULL) HICR_THROW_LOGIC("The passed compute unit is not supported by this compute manager\n");

    // Getting core affinity
    auto cpuId = core->getAffinity();

    hwloc_obj_t pu = hwloc_get_obj_by_type(*_topology, HWLOC_OBJ_PU, cpuId);
    hwloc_obj_t obj = pu;
    if (!obj) HICR_THROW_RUNTIME("Attempting to access a compute resource that does not exist (%lu) in this backend", cpuId);

    std::vector<unsigned> ret;

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

  /**
   * Uses HWloc to discover the NUMA node associated with a given logical processor ID
   *
   * \param[in] cpuId The ID of the processor we are doing the search for
   * \returns The ID of the associated memory space
   */
  size_t getCpuNumaAffinity(HiCR::L0::ComputeResource* computeResource) const
  {
    // Getting up-casted pointer for the MPI instance
    auto core = dynamic_cast<L0::ComputeResource *const>(computeResource);

    // Checking whether the execution unit passed is compatible with this backend
    if (core == NULL) HICR_THROW_LOGIC("The passed compute unit is not supported by this compute manager\n");

    // Getting core affinity
    auto cpuId = core->getAffinity();

    // Sanitize input? So far we only call it internally so assume ID given is safe?
    hwloc_obj_t obj = hwloc_get_obj_by_type(*_topology, HWLOC_OBJ_PU, cpuId);

    if (!obj) HICR_THROW_RUNTIME("Attempting to access a compute resource that does not exist (%lu) in this backend", cpuId);

    size_t ret = 0;

    // obj is a leaf/PU; get to its parents to discover the hwloc memory space it belongs to
    hwloc_obj_t ancestor = obj->parent;
    hwloc_obj_t nodeNUMA = nullptr;
    bool found = false;

    // iterate over parents until we find a memory node
    while (ancestor && !ancestor->memory_arity)
      ancestor = ancestor->parent;

    // iterate over potential sibling nodes (the likely behavior though is to run only once)
    for (size_t memChild = 0; memChild < ancestor->memory_arity; memChild++)
    {
      if (memChild == 0)
        nodeNUMA = ancestor->memory_first_child;
      else if (nodeNUMA)
        nodeNUMA = nodeNUMA->next_sibling;

      if (hwloc_obj_type_is_memory(nodeNUMA->type) &&
          hwloc_bitmap_isset(obj->nodeset, nodeNUMA->os_index))
      {
        found = true;
        ret = nodeNUMA->logical_index;
        break;
      }
    }

    if (!found) HICR_THROW_RUNTIME("NUMA Node not detected for compute resource (%lu)", cpuId);

    return ret;
  }

  /**
   * Uses HWloc to discover all caches associated with a given logical processor ID
   *
   * \param[in] cpuId The ID of the processor we are doing the search for
   * \returns A vector of (type,size) entries, where type is a string describing
   *          the cache and size is the respective size in Bytes
   *          The 'type' string is has the following form:
   *          "Level <I/D/U> <P/S> <associated IDs>", where:
   *          Level: may be "L1", "L2, "L3"
   *          I/D/U: may be "Instruction", "Data", "Unified"
   *          P/S:   may be "Private" or "Shared"
   *          associated IDs: (optional, for Shared cache) a list of core IDs, e.g. "0 1 2 3"
   */
  std::vector<std::pair<std::string, size_t>> getCpuCaches(HiCR::L0::ComputeResource* computeResource) const
  {
    // Getting up-casted pointer for the MPI instance
    auto core = dynamic_cast<L0::ComputeResource *const>(computeResource);

    // Checking whether the execution unit passed is compatible with this backend
    if (core == NULL) HICR_THROW_LOGIC("The passed compute unit is not supported by this compute manager\n");

    // Getting core affinity
    auto cpuId = core->getAffinity();

    // Sanitize input? So far we only call it internally so assume ID given is safe?
    hwloc_obj_t obj = hwloc_get_obj_by_type(*_topology, HWLOC_OBJ_PU, cpuId);

    if (!obj) HICR_THROW_RUNTIME("Attempting to access a compute resource that does not exist (%lu) in this backend", cpuId);

    std::vector<std::pair<std::string, size_t>> ret;
    std::string type;
    size_t size;

    // Start from 1 level above our leaf/PU
    hwloc_obj_t cache = obj->parent;
    do {
      // Check if the current object is a cache-type object
      if (cache->type == HWLOC_OBJ_L1CACHE || cache->type == HWLOC_OBJ_L2CACHE || cache->type == HWLOC_OBJ_L3CACHE || cache->type == HWLOC_OBJ_L4CACHE || cache->type == HWLOC_OBJ_L5CACHE || cache->type == HWLOC_OBJ_L1ICACHE || cache->type == HWLOC_OBJ_L2ICACHE || cache->type == HWLOC_OBJ_L3ICACHE)
      {
        // In case it is a cache, deduce the level from the types HWloc supports
        switch (cache->type)
        {
        case HWLOC_OBJ_L1CACHE:
        case HWLOC_OBJ_L1ICACHE:
          type = "L1";
          break;
        case HWLOC_OBJ_L2CACHE:
        case HWLOC_OBJ_L2ICACHE:
          type = "L2";
          break;
        case HWLOC_OBJ_L3CACHE:
        case HWLOC_OBJ_L3ICACHE:
          type = "L3";
          break;
        case HWLOC_OBJ_L4CACHE:
          type = "L4";
          break;
        case HWLOC_OBJ_L5CACHE:
          type = "L5";
          break;
        // We never expect to get here; this is for compiler warning suppresion
        default:
          type = "Unknown level";
        }

        type += " ";

        // Discover the type: Instruction, Data or Unified
        switch (cache->attr->cache.type)
        {
        case HWLOC_OBJ_CACHE_UNIFIED:
          type += "Unified";
          break;
        case HWLOC_OBJ_CACHE_INSTRUCTION:
          type += "Instruction";
          break;
        case HWLOC_OBJ_CACHE_DATA:
          type += "Data";
          break;
        }

        type += " ";

        // Discover if the cache is private to a core or shared, via the arity field
        // If shared, discover and export the PU IDs that share it
        if (cache->arity > 1)
        {
          type += "Shared";
          for (size_t i = 0; i < cache->arity; i++)
          {
            hwloc_obj_t child = cache->children[i];
            std::vector<int> puIds;
            getThreadPUs(*_topology, child, 0, puIds);
            for (int id : puIds)
              type += " " + std::to_string(id);
          }
        }
        else
          type += "Private";

        // Get size
        size = cache->attr->cache.size;
        // Insert element to our return container
        ret.push_back(std::make_pair(type, size));
      }

      // Repeat the search 1 level above
      cache = cache->parent;
    } while (cache);

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
    std::vector<int> threadPUs;
    getThreadPUs(*_topology, hwloc_get_root_obj(*_topology), 0, threadPUs);
    for (const auto pu : threadPUs) computeResourceList.insert(new L0::ComputeResource(pu));

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
