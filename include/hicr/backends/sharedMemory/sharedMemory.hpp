// This is a minimal backend for multi-core support based on OpenMP

#pragma once

#include <errno.h>
#include <memory>
#include <stdio.h>
#include <string.h>

#include "hwloc.h"

#include <hicr/backend.hpp>
#include <hicr/backends/sharedMemory/thread.hpp>

namespace HiCR
{

namespace backend
{

namespace sharedMemory
{

/**
 * Implementation of the Pthreads/HWloc-based HiCR Shared Memory Backend.
 *
 * It detects and returns the processing units and memory spaces detected by the HWLoc library, and instantiates the former as pthreads and the latter as MemorySpace descriptors. It also detects their connectivity.
 */
class SharedMemory : public Backend
{
  private:
  /**
   * Local processor and memory hierarchy topology, as detected by Hwloc
   */
  hwloc_topology_t _topology;

  public:
  SharedMemory() = default;
  ~SharedMemory() = default;

  /**
   * Uses HWloc to recursively (tree-like) identify the system's basic processing units (PUs)
   *
   * \param[in] topology An HWLoc topology object, already initialized
   * \param[in] obj The root HWLoc object for the start of the exploration tree at every recursion level
   * \param[in] depth Stores the current exploration depth level, necessary to return only the processing units at the leaf level
   * \param[out] threadPUs Storage for the found procesing units
   */
  static void getThreadPUs(hwloc_topology_t topology, hwloc_obj_t obj, int depth, std::vector<int> &threadPUs)
  {
    if (obj->arity == 0) threadPUs.push_back(obj->os_index);
    for (unsigned int i = 0; i < obj->arity; i++) getThreadPUs(topology, obj->children[i], depth + 1, threadPUs);
  }

  /**
   * Pthread implementation of the Backend queryResources() function. This will add one resource object per found Thread / Processing Unit (PU)
   */
  void queryResources() override
  {
    hwloc_topology_init(&_topology);
    hwloc_topology_load(_topology);

    std::vector<int> threadPUs;
    getThreadPUs(_topology, hwloc_get_root_obj(_topology), 0, threadPUs);

    // Creating Thread objects
    for (size_t i = 0; i < threadPUs.size(); i++)
    {
      auto affinity = std::vector<int>({threadPUs[i]});
      auto thread = std::make_unique<Thread>(i, affinity);
      _resourceList.push_back(std::move(thread));
    }
  }
};

} // namespace sharedMemory

} // namespace backend

} // namespace HiCR
