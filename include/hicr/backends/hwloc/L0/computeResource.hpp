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
#include <hwloc.h>
#include <hicr/backends/hwloc/cache.hpp>
#include <hicr/core/definitions.hpp>
#include <hicr/core/exceptions.hpp>
#include <hicr/core/L0/computeResource.hpp>

namespace HiCR::backend::hwloc::L0
{

/**
 * This class represents a compute resource, visible by HWLoc.
 * That is, a CPU processing unit (core or hyperthread) with information about caches and locality.
 */
class ComputeResource final : public HiCR::L0::ComputeResource
{
  public:

  /**
   * System-given logical processor (core or hyperthread) identifier that this class instance represents
   */
  using logicalProcessorId_t = unsigned int;

  /**
   * System-given physical processor identifier that this class instance represents
   */
  using physicalProcessorId_t = unsigned int;

  /**
   * System-given NUMA affinity identifier
   */
  using numaAffinity_t = unsigned int;

  /**
   * Constructor for the compute resource class of the hwloc backend
   * \param topology HWLoc topology object for discovery
   * \param logicalProcessorId Os-determied core affinity assigned to this compute resource
   */
  ComputeResource(hwloc_topology_t topology, const logicalProcessorId_t logicalProcessorId)
    : HiCR::L0::ComputeResource(),
      _logicalProcessorId(logicalProcessorId),
      _physicalProcessorId(detectPhysicalProcessorId(topology, logicalProcessorId)),
      _numaAffinity(detectCoreNUMAffinity(topology, logicalProcessorId)),
      _caches(detectCpuCaches(topology, logicalProcessorId)){};

  /**
   * Constructor for the compute resource class of the hwloc backend
   * \param[in] logicalProcessorId Unique identifier for the core assigned to this compute resource
   * \param[in] numaAffinity The NUMA domain associated to this core
   * \param[in] caches The set of caches contained to or accessible by this core
   * \param[in] physicalProcessorId The identifier of the physical core as assigned by the OS
   */
  ComputeResource(const logicalProcessorId_t                                 logicalProcessorId,
                  const physicalProcessorId_t                                physicalProcessorId,
                  const numaAffinity_t                                       numaAffinity,
                  std::unordered_set<std::shared_ptr<backend::hwloc::Cache>> caches)
    : HiCR::L0::ComputeResource(),
      _logicalProcessorId(logicalProcessorId),
      _physicalProcessorId(physicalProcessorId),
      _numaAffinity(numaAffinity),
      _caches(std::move(caches)){};
  ~ComputeResource() override = default;

  /**
   * Default constructor for serialization/deserialization purposes
   */
  ComputeResource() = default;

  __INLINE__ std::string getType() const override { return "Processing Unit"; }

  /**
   * Function to return the compute resource processor id
   *
   * @returns The processor id
   */
  __INLINE__ logicalProcessorId_t getProcessorId() const { return _logicalProcessorId; }

  /**
   * Obtains the Core ID of the CPU; in non SMT systems that will be the actual id;
   * in SMT it is the id of the actual core the thread belongs to.
   *
   * \return The physical ID of the hardware Core
   */
  __INLINE__ physicalProcessorId_t getPhysicalProcessorId() const { return _physicalProcessorId; }

  /**
   * Deserializing constructor
   *
   * The instance created will contain all information, if successful in deserializing it, corresponding to the passed processing unit
   * This instance should NOT be used for anything else than reporting/printing the contained resources
   *
   * @param[in] input A JSON-encoded serialized  processing unit information
   */
  ComputeResource(const nlohmann::json &input) { deserialize(input); }

  /**
   * Uses HWloc to recursively (tree-like) identify the host's basic processing units (PUs)
   *
   * \param[in] topology An HWLoc topology object, already initialized
   * \param[in] obj The root HWLoc object for the start of the exploration tree at every recursion level
   * \param[in] depth Stores the current exploration depth level, necessary to return only the processing units at the leaf level
   * \param[out] threadPUs Storage for the found procesing units
   */
  __INLINE__ static void detectThreadPUs(hwloc_topology_t topology, hwloc_obj_t obj, int depth, std::vector<logicalProcessorId_t> &threadPUs)
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
  __INLINE__ static physicalProcessorId_t detectPhysicalProcessorId(hwloc_topology_t topology, const logicalProcessorId_t logicalProcessorId)
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
  __INLINE__ static numaAffinity_t detectCoreNUMAffinity(hwloc_topology_t topology, const logicalProcessorId_t logicalProcessorId)
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
    while (ancestor && !ancestor->memory_arity) ancestor = ancestor->parent;

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
  __INLINE__ static std::unordered_set<std::shared_ptr<backend::hwloc::Cache>> detectCpuCaches(hwloc_topology_t topology, const logicalProcessorId_t logicalProcessorId)
  {
    // Sanitize input? So far we only call it internally so assume ID given is safe?
    hwloc_obj_t obj = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, logicalProcessorId);

    if (!obj) HICR_THROW_RUNTIME("Attempting to access a compute resource that does not exist (%lu) in this backend", logicalProcessorId);

    std::unordered_set<std::shared_ptr<backend::hwloc::Cache>> ret;

    // Start from 1 level above our leaf/PU
    hwloc_obj_t cache = obj->parent;
    while (cache)
    {
      Cache::cacheLevel_t level = Cache::cacheLevel_t::L1;
      std::string         type;

      // Check if the current object is a cache-type object
      if (cache->type == HWLOC_OBJ_L1CACHE || cache->type == HWLOC_OBJ_L2CACHE || cache->type == HWLOC_OBJ_L3CACHE || cache->type == HWLOC_OBJ_L4CACHE ||
          cache->type == HWLOC_OBJ_L5CACHE || cache->type == HWLOC_OBJ_L1ICACHE || cache->type == HWLOC_OBJ_L2ICACHE || cache->type == HWLOC_OBJ_L3ICACHE)
      {
        // In case it is a cache, deduce the level from the types HWloc supports
        switch (cache->type)
        {
        case HWLOC_OBJ_L1CACHE:
        case HWLOC_OBJ_L1ICACHE: level = Cache::cacheLevel_t::L1; break;
        case HWLOC_OBJ_L2CACHE:
        case HWLOC_OBJ_L2ICACHE: level = Cache::cacheLevel_t::L2; break;
        case HWLOC_OBJ_L3CACHE:
        case HWLOC_OBJ_L3ICACHE: level = Cache::cacheLevel_t::L3; break;
        case HWLOC_OBJ_L4CACHE: level = Cache::cacheLevel_t::L4; break;
        case HWLOC_OBJ_L5CACHE: level = Cache::cacheLevel_t::L5; break;
        // We never expect to get here; this is for compiler warning suppresion
        default: HICR_THROW_RUNTIME("Unsupported Cache level detected (%lu)", cache->type);
        }

        // Storage for cache type
        std::string type = "Unknown";

        // Discover the type: Instruction, Data or Unified
        switch (cache->attr->cache.type)
        {
        case HWLOC_OBJ_CACHE_UNIFIED: type = "Unified"; break;
        case HWLOC_OBJ_CACHE_INSTRUCTION: type = "Instruction"; break;
        case HWLOC_OBJ_CACHE_DATA: type = "Data"; break;
        }

        // Storage for more cache information
        const bool shared   = cache->arity > 1;
        const auto size     = cache->attr->cache.size;
        const auto lineSize = cache->attr->cache.linesize;

        // Insert element to our return container
        ret.insert(std::make_shared<backend::hwloc::Cache>(level, type, size, lineSize, shared));
      }

      // Repeat the search 1 level above
      cache = cache->parent;
    }

    return ret;
  }

  /**
   * Uses HWloc to discover the NUMA node associated with a given logical processor ID
   *
   * \param[in] topology HWLoc topology object
   * \param[in] logicalProcessorId The ID of the processor we are doing the search for
   * \returns The ID of the associated memory space
   */
  __INLINE__ static numaAffinity_t getCpuNumaAffinity(hwloc_topology_t topology, const logicalProcessorId_t logicalProcessorId)
  {
    // Sanitize input? So far we only call it internally so assume ID given is safe?
    hwloc_obj_t obj = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, logicalProcessorId);

    if (!obj) HICR_THROW_RUNTIME("Attempting to access a compute resource that does not exist (%lu) in this backend", logicalProcessorId);

    numaAffinity_t ret = 0;

    // obj is a leaf/PU; get to its parents to discover the hwloc memory space it belongs to
    hwloc_obj_t ancestor = obj->parent;
    hwloc_obj_t nodeNUMA = nullptr;
    bool        found    = false;

    // iterate over parents until we find a memory node
    while (ancestor && !ancestor->memory_arity) ancestor = ancestor->parent;

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
        ret   = (numaAffinity_t)nodeNUMA->logical_index;
        break;
      }
    }

    if (found == false) HICR_THROW_RUNTIME("NUMA Domain not detected for compute resource (%lu)", logicalProcessorId);

    return ret;
  }

  protected:

  __INLINE__ void serializeImpl(nlohmann::json &output) const override
  {
    // Writing core's information into the serialized object
    output["Logical Processor Id"]  = _logicalProcessorId;
    output["Physical Processor Id"] = _physicalProcessorId;
    output["NUMA Affinity"]         = _numaAffinity;

    // Writing Cache information
    std::string cachesKey = "Caches";
    output[cachesKey]     = std::vector<nlohmann::json>();
    for (const auto &cache : _caches) output[cachesKey] += cache->serialize();
  }

  __INLINE__ void deserializeImpl(const nlohmann::json &input) override
  {
    std::string key = "Logical Processor Id";
    if (input.contains(key) == false) HICR_THROW_LOGIC("The serialized object contains no '%s' key", key.c_str());
    if (input[key].is_number() == false) HICR_THROW_LOGIC("The '%s' entry is not a number", key.c_str());
    _logicalProcessorId = input[key].get<logicalProcessorId_t>();

    key = "Physical Processor Id";
    if (input.contains(key) == false) HICR_THROW_LOGIC("The serialized object contains no '%s' key", key.c_str());
    if (input[key].is_number() == false) HICR_THROW_LOGIC("The '%s' entry is not a number", key.c_str());
    _physicalProcessorId = input[key].get<physicalProcessorId_t>();

    key = "NUMA Affinity";
    if (input.contains(key) == false) HICR_THROW_LOGIC("The serialized object contains no '%s' key", key.c_str());
    if (input[key].is_number() == false) HICR_THROW_LOGIC("The '%s' entry is not a number", key.c_str());
    _numaAffinity = input[key].get<numaAffinity_t>();

    key = "Caches";
    if (input.contains(key) == false) HICR_THROW_LOGIC("The serialized object contains no '%s' key", key.c_str());
    if (input[key].is_array() == false) HICR_THROW_LOGIC("The '%s' entry is not an array", key.c_str());

    _caches.clear();
    for (const auto &c : input[key])
    {
      // Deserializing cache
      auto cache = std::make_shared<backend::hwloc::Cache>(c);

      // Adding it to the list
      _caches.insert(cache);
    }
  }

  private:

  /**
   * The logical ID of the hardware core / processing unit
   */
  logicalProcessorId_t _logicalProcessorId{};

  /**
   * The ID of the hardware core; in SMT systems that will mean the core ID,
   * which may also have other HW threads. In non SMT systems it is expected
   * for logical and system IDs to be 1-to-1.
   */
  physicalProcessorId_t _physicalProcessorId{};

  /**
   * The ID of the hardware NUMA domain that this core is associated to
   */
  numaAffinity_t _numaAffinity{};

  /**
   * List of Cache objects associated with the CPU. There is the assumption
   * that only one cache object of each type can be associated with a CPU.
   */
  std::unordered_set<std::shared_ptr<backend::hwloc::Cache>> _caches;
};

} // namespace HiCR::backend::hwloc::L0
