/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file computeManager.hpp
 * @brief This is a minimal backend for compute management of (multi-threaded) shared memory systems
 * @author S. M. Martin
 * @date 10/12/2023
 */

#pragma once

#include <memory>
#include <filesystem>
#include "hwloc.h"
#include <hicr/backends/computeManager.hpp>
#include <hicr/backends/sequential/executionUnit.hpp>
#include <hicr/backends/sharedMemory/processingUnit.hpp>

namespace HiCR
{

namespace backend
{

namespace sharedMemory
{

/**
 * Implementation of the HWloc-based HiCR Shared Memory backend's compute manager.
 *
 * It detects and returns the processing units detected by the HWLoc library
 */
class ComputeManager final : public backend::ComputeManager
{
  public:

  /**
   * Constructor for the compute manager class for the shared memory backend
   *
   * \param[in] topology An HWloc topology object that can be used to query the available computational resources
   */
  ComputeManager(hwloc_topology_t *topology) : backend::ComputeManager(), _topology{topology} {}

  /**
   * The constructor is employed to free memory required for hwloc
   */
  ~ComputeManager() = default;

  __USED__ inline ExecutionUnit *createExecutionUnit(sequential::ExecutionUnit::function_t executionUnit) override
  {
    return new sequential::ExecutionUnit(executionUnit);
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
    if (obj->arity == 0) threadPUs.push_back(obj->os_index);
    for (unsigned int i = 0; i < obj->arity; i++) getThreadPUs(topology, obj->children[i], depth + 1, threadPUs);
  }

  private:

  /**
   * Pthread implementation of the Backend queryResources() function. This will add one compute resource object per Thread / Processing Unit (PU) found
   */
  __USED__ inline computeResourceList_t queryComputeResourcesImpl() override
  {
    // Loading topology
    hwloc_topology_load(*_topology);

    // New compute resource list to return
    computeResourceList_t computeResourceList;

    // Creating compute resource list, based on the  processing units (hyperthreads) observed by HWLoc
    std::vector<int> threadPUs;
    getThreadPUs(*_topology, hwloc_get_root_obj(*_topology), 0, threadPUs);
    computeResourceList.insert(threadPUs.begin(), threadPUs.end());

    // Returning new compute resource list
    return computeResourceList;
  }

  __USED__ inline std::unique_ptr<HiCR::ProcessingUnit> createProcessingUnitImpl(computeResourceId_t resource) const override
  {
    return std::make_unique<ProcessingUnit>(resource);
  }

  /**
   * Local processor and memory hierarchy topology, as detected by Hwloc
   */
  hwloc_topology_t *_topology;
};

} // namespace sharedMemory
} // namespace backend
} // namespace HiCR
