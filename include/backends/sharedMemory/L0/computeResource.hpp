/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file executionUnit.hpp
 * @brief This file implements the compute resource class for the sequential backend
 * @author S. M. Martin & O. Korakitis
 * @date 12/12/2023
 */

#pragma once

#include "hwloc.h"
#include <backends/sharedMemory/cache.hpp>
#include <hicr/L0/computeResource.hpp>
#include <hicr/common/definitions.hpp>
#include <hicr/common/exceptions.hpp>

namespace HiCR
{

namespace backend
{

namespace sharedMemory
{

namespace L0
{

/**
 * This class represents a compute resource, visible by the sequential backend. That is, a CPU processing unit (core or hyperthread) with information about locality.
 */
class ComputeResource final : public HiCR::L0::ComputeResource
{
  public:

  /**
   * System-given logical processor (core or hyperthread) identifier that this class instance represents
  */
  typedef int logicalProcessorId_t;

  /**
   * System-given physical processor identifier that this class instance represents
  */
  typedef int physicalProcessorId_t;

  /**
   * System-given NUMA affinity identifier
  */
  typedef int numaAffinity_t;

  /**
   * Constructor for the compute resource class of the sequential backend
   *
   * \param affinity Os-determied core affinity assigned to this compute resource
   */
  ComputeResource(hwloc_topology_t topology, const logicalProcessorId_t logicalProcessorId) :
   HiCR::L0::ComputeResource(),
    _logicalProcessorId(logicalProcessorId),
    _physicalProcessorId(getPhysicalProcessorId(topology, logicalProcessorId)),
    _numaAffinity(getCoreNUMAffinity(topology, logicalProcessorId)),
    _caches(getCpuCaches(topology, logicalProcessorId))
     {};
  ComputeResource() = delete;

  /**
   * Default destructor
   */
  ~ComputeResource() = default;

  __USED__ inline std::string getType() const override { return "CPU Core"; }

  /**
   * Function to return the compute resource processor id
   * 
   * @returns The processor id
  */
  __USED__ inline int getProcessorId() const { return _logicalProcessorId; }

  /**
   * Obtains the Core ID of the CPU; in non SMT systems that will be the actual id;
   * in SMT it is the id of the actual core the thread belongs to.
   *
   * \return The physical ID of the hardware Core
   */
  __USED__ inline unsigned int getPhysicalProcessorId() const { return _physicalProcessorId; }

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
   * Uses HWloc to discover the (physical) processor ID, associated with a given logical processor ID
   *
   * \param[in] logicalProcessorId The logical ID of the processor we are doing the search for
   * \returns The ID of the associated physical identifier related to the passed logical processor id
   */
  __USED__ inline static physicalProcessorId_t getPhysicalProcessorId(hwloc_topology_t topology, const logicalProcessorId_t logicalProcessorId)
  {
    hwloc_obj_t obj = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, logicalProcessorId);
    if (!obj)  HICR_THROW_RUNTIME( "Attempting to access a compute resource that does not exist (%lu) in this backend", logicalProcessorId);

    // Acquire the parent core object
    // There is an asumption here that a HWLOC_OBJ_PU type always has a parent of type HWLOC_OBJ_CORE,
    // which is consistent with current HWloc, but maybe reconsider it.
    obj = obj->parent;
    if (obj->type != HWLOC_OBJ_CORE)  HICR_THROW_RUNTIME("Unexpected hwloc object type while trying to access Core/CPU (%lu)", logicalProcessorId);

    return obj->logical_index;
  }

  /**
   * Uses HWloc to discover the NUMA node associated with a given logical processor ID
   *
   * \param[in] cpuId The ID of the processor we are doing the search for
   * \returns The ID of the associated memory space
   */
  __USED__ static numaAffinity_t getCoreNUMAffinity(hwloc_topology_t topology, const logicalProcessorId_t logicalProcessorId)
  {
    // Sanitize input? So far we only call it internally so assume ID given is safe?
    hwloc_obj_t obj = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, logicalProcessorId);

    if (!obj) HICR_THROW_RUNTIME("Attempting to access a compute resource that does not exist (%lu) in this backend", logicalProcessorId);

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

    if (!found) HICR_THROW_RUNTIME("NUMA Node not detected for compute resource (%lu)", logicalProcessorId);

    return ret;
  }

/**
   * Uses HWloc to discover all caches associated with a given logical processor ID
   *
   * \param[in] logicalProcessorId The logical ID of the processor we are doing the search for
   * \returns A vector of (type,size) entries, where type is a string describing
   *          the cache and size is the respective size in Bytes
   *          The 'type' string is has the following form:
   *          "Level <I/D/U> <P/S> <associated IDs>", where:
   *          Level: may be "L1", "L2, "L3"
   *          I/D/U: may be "Instruction", "Data", "Unified"
   *          P/S:   may be "Private" or "Shared"
   *          associated IDs: (optional, for Shared cache) a list of core IDs, e.g. "0 1 2 3"
   */
  __USED__ static inline std::vector<backend::sharedMemory::Cache> getCpuCaches(hwloc_topology_t topology, const logicalProcessorId_t logicalProcessorId)
  {
    // Sanitize input? So far we only call it internally so assume ID given is safe?
    hwloc_obj_t obj = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, logicalProcessorId);

    if (!obj) HICR_THROW_RUNTIME("Attempting to access a compute resource that does not exist (%lu) in this backend", logicalProcessorId);

    std::vector<backend::sharedMemory::Cache> ret;
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
            getThreadPUs(topology, child, 0, puIds);
            for (int id : puIds)
              type += " " + std::to_string(id);
          }
        }
        else
          type += "Private";

        // Get size
        size = cache->attr->cache.size; 

        // Insert element to our return container
        ret.push_back(backend::sharedMemory::Cache(type, size));
      }

      // Repeat the search 1 level above
      cache = cache->parent;
    } while (cache);

    return ret;
  }

  private:

   /**
   * The logical ID of the hardware core / processing unit
   */
  const logicalProcessorId_t _logicalProcessorId;

   /**
   * The ID of the hardware core; in SMT systems that will mean the core ID,
   * which may also have other HW threads. In non SMT systems it is expected
   * for logical and system IDs to be 1-to-1.
   */
  const physicalProcessorId_t _physicalProcessorId;

   /**
   * The ID of the hardware NUMA domain that this core is associated to
   */
  const numaAffinity_t _numaAffinity;

   /**
   * List of Cache objects associated with the CPU. There is the assumption
   * that only one cache object of each type can be associated with a CPU.
   */
  const std::vector<backend::sharedMemory::Cache> _caches;
};

} // namespace L0

} // namespace sharedMemory

} // namespace backend

} // namespace HiCR
