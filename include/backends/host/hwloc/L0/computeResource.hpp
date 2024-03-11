/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file computeResource.hpp
 * @brief This file implements the compute resource class for the HWLoc-based backend
 * @author O. Korakitis & S. M. Martin
 * @date 12/12/2023
 */

#pragma once

#include <unordered_set>
#include "hwloc.h"
#include <hicr/definitions.hpp>
#include <hicr/exceptions.hpp>
#include <backends/host/L0/computeResource.hpp>

namespace HiCR
{

namespace backend
{

namespace host
{

namespace hwloc
{

namespace L0
{

/**
 * This class represents a compute resource, visible by HWLoc.
 * That is, a CPU processing unit (core or hyperthread) with information about caches and locality.
 */
class ComputeResource final : public HiCR::backend::host::L0::ComputeResource
{
  public:

  /**
   * Constructor for the compute resource class of the sequential backend
   * \param topology HWLoc topology object for discovery
   * \param logicalProcessorId Os-determied core affinity assigned to this compute resource
   */
  ComputeResource(hwloc_topology_t topology, const logicalProcessorId_t logicalProcessorId)
    : HiCR::backend::host::L0::ComputeResource(logicalProcessorId,
                                               detectPhysicalProcessorId(topology, logicalProcessorId),
                                               detectCoreNUMAffinity(topology, logicalProcessorId),
                                               detectCpuCaches(topology, logicalProcessorId)){};

  /**
   * Deserializing constructor
   *
   * The instance created will contain all information, if successful in deserializing it, corresponding to the passed processing unit
   * This instance should NOT be used for anything else than reporting/printing the contained resources
   *
   * @param[in] input A JSON-encoded serialized  processing unit information
   */
  ComputeResource(const nlohmann::json &input)
  {
    deserialize(input);
  }

  /**
   * Default destructor
   */
  ~ComputeResource() = default;

  /**
   * Uses HWloc to recursively (tree-like) identify the host's basic processing units (PUs)
   *
   * \param[in] topology An HWLoc topology object, already initialized
   * \param[in] obj The root HWLoc object for the start of the exploration tree at every recursion level
   * \param[in] depth Stores the current exploration depth level, necessary to return only the processing units at the leaf level
   * \param[out] threadPUs Storage for the found procesing units
   */
  __USED__ inline static void detectThreadPUs(hwloc_topology_t topology, hwloc_obj_t obj, int depth, std::vector<int> &threadPUs)
  {
    if (obj->arity == 0) threadPUs.push_back(obj->logical_index);
    for (unsigned int i = 0; i < obj->arity; i++) detectThreadPUs(topology, obj->children[i], depth + 1, threadPUs);
  }

  /**
   * Uses HWloc to discover the (physical) processor ID, associated with a given logical processor ID
   *
   * \param[in] topology An HWLoc topology object, already initialized
   * \param[in] logicalProcessorId The logical ID of the processor we are doing the search for
   * \returns The ID of the associated physical identifier related to the passed logical processor id
   */
  __USED__ inline static physicalProcessorId_t detectPhysicalProcessorId(hwloc_topology_t topology, const logicalProcessorId_t logicalProcessorId)
  {
    hwloc_obj_t obj = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, logicalProcessorId);
    if (!obj) HICR_THROW_RUNTIME("Attempting to access a compute resource that does not exist (%lu) in this backend", logicalProcessorId);

    // Acquire the parent core object
    // There is an asumption here that a HWLOC_OBJ_PU type always has a parent of type HWLOC_OBJ_CORE,
    // which is consistent with current HWloc, but maybe reconsider it.
    obj = obj->parent;
    if (obj->type != HWLOC_OBJ_CORE) HICR_THROW_RUNTIME("Unexpected hwloc object type while trying to access Core/CPU (%lu)", logicalProcessorId);

    return obj->logical_index;
  }

  /**
   * Uses HWloc to discover the NUMA node associated with a given logical processor ID
   *
   * \param[in] topology An HWLoc topology object, already initialized
   * \param[in] logicalProcessorId The ID of the processor we are doing the search for
   * \returns The ID of the associated memory space
   */
  __USED__ static numaAffinity_t detectCoreNUMAffinity(hwloc_topology_t topology, const logicalProcessorId_t logicalProcessorId)
  {
    // Sanitize input? So far we only call it internally so assume ID given is safe?
    hwloc_obj_t obj = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, logicalProcessorId);

    if (!obj) HICR_THROW_RUNTIME("Attempting to access a compute resource that does not exist (%lu) in this backend", logicalProcessorId);

    size_t ret = 0;

    // obj is a leaf/PU; get to its parents to discover the hwloc memory space it belongs to
    hwloc_obj_t ancestor = obj->parent;
    hwloc_obj_t nodeNUMA = nullptr;
    bool        found    = false;

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
        ret   = nodeNUMA->logical_index;
        break;
      }
    }

    if (!found) HICR_THROW_RUNTIME("NUMA Domain not detected for compute resource (%lu)", logicalProcessorId);

    return ret;
  }

  /**
   * Uses HWloc to discover all caches associated with a given logical processor ID
   *
   * \param[in] topology An HWLoc topology object, already initialized
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
  __USED__ static inline std::unordered_set<std::shared_ptr<backend::host::Cache>> detectCpuCaches(hwloc_topology_t topology, const logicalProcessorId_t logicalProcessorId)
  {
    // Sanitize input? So far we only call it internally so assume ID given is safe?
    hwloc_obj_t obj = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, logicalProcessorId);

    if (!obj) HICR_THROW_RUNTIME("Attempting to access a compute resource that does not exist (%lu) in this backend", logicalProcessorId);

    std::unordered_set<std::shared_ptr<backend::host::Cache>> ret;

    // Start from 1 level above our leaf/PU
    hwloc_obj_t cache = obj->parent;
    while (cache)
    {
      Cache::cacheLevel_t level;
      std::string         type;

      // Check if the current object is a cache-type object
      if (cache->type == HWLOC_OBJ_L1CACHE || cache->type == HWLOC_OBJ_L2CACHE || cache->type == HWLOC_OBJ_L3CACHE || cache->type == HWLOC_OBJ_L4CACHE || cache->type == HWLOC_OBJ_L5CACHE || cache->type == HWLOC_OBJ_L1ICACHE || cache->type == HWLOC_OBJ_L2ICACHE || cache->type == HWLOC_OBJ_L3ICACHE)
      {
        // In case it is a cache, deduce the level from the types HWloc supports
        switch (cache->type)
        {
        case HWLOC_OBJ_L1CACHE:
        case HWLOC_OBJ_L1ICACHE:
          level = 1;
          break;
        case HWLOC_OBJ_L2CACHE:
        case HWLOC_OBJ_L2ICACHE:
          level = 2;
          break;
        case HWLOC_OBJ_L3CACHE:
        case HWLOC_OBJ_L3ICACHE:
          level = 3;
          break;
        case HWLOC_OBJ_L4CACHE:
          level = 4;
          break;
        case HWLOC_OBJ_L5CACHE:
          level = 5;
          break;
        // We never expect to get here; this is for compiler warning suppresion
        default:
          level = 0;
        }

        // Storage for cache type
        std::string type = "Unknown";

        // Discover the type: Instruction, Data or Unified
        switch (cache->attr->cache.type)
        {
        case HWLOC_OBJ_CACHE_UNIFIED:
          type = "Unified";
          break;
        case HWLOC_OBJ_CACHE_INSTRUCTION:
          type = "Instruction";
          break;
        case HWLOC_OBJ_CACHE_DATA:
          type = "Data";
          break;
        }

        // Storage for more cache information
        const bool shared   = cache->arity > 1;
        const auto size     = cache->attr->cache.size;
        const auto lineSize = cache->attr->cache.linesize;

        // Insert element to our return container
        ret.insert(std::make_shared<backend::host::Cache>(level, type, size, lineSize, shared));
      }

      // Repeat the search 1 level above
      cache = cache->parent;
    }

    return ret;
  }

  __USED__ inline void serializeImpl(nlohmann::json &output) const override
  {
    // Calling the base class serializer
    this->HiCR::backend::host::L0::ComputeResource::serializeImpl(output);
  }

  __USED__ inline void deserializeImpl(const nlohmann::json &input) override
  {
    // Calling the base class deserializer
    this->HiCR::backend::host::L0::ComputeResource::deserializeImpl(input);
  }

  /**
   * Uses HWloc to discover the NUMA node associated with a given logical processor ID
   *
   * \param[in] topology HWLoc topology object
   * \param[in] logicalProcessorId The ID of the processor we are doing the search for
   * \returns The ID of the associated memory space
   */
  __USED__ inline static int getCpuNumaAffinity(hwloc_topology_t topology, const logicalProcessorId_t logicalProcessorId)
  {
    // Sanitize input? So far we only call it internally so assume ID given is safe?
    hwloc_obj_t obj = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, logicalProcessorId);

    if (!obj) HICR_THROW_RUNTIME("Attempting to access a compute resource that does not exist (%lu) in this backend", logicalProcessorId);

    int ret = -1;

    // obj is a leaf/PU; get to its parents to discover the hwloc memory space it belongs to
    hwloc_obj_t ancestor = obj->parent;
    hwloc_obj_t nodeNUMA = nullptr;
    bool        found    = false;

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

      if (hwloc_obj_type_is_memory(nodeNUMA->type) && hwloc_bitmap_isset(obj->nodeset, nodeNUMA->os_index))
      {
        found = true;
        ret   = nodeNUMA->logical_index;
        break;
      }
    }

    if (found == false) HICR_THROW_RUNTIME("NUMA Domain not detected for compute resource (%lu)", logicalProcessorId);

    return ret;
  }
};

} // namespace L0

} // namespace hwloc

} // namespace host

} // namespace backend

} // namespace HiCR
