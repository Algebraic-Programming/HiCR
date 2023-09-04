/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file pthreads.hpp
 * @brief This is a minimal backend for shared memory multi-core support based on HWLoc and Pthreads
 * @author S. M. Martin
 * @date 14/8/2023
 */

#pragma once

#include <cstring>
#include <errno.h>
#include <future>
#include <memory>
#include <stdio.h>

#include "hwloc.h"

#include <hicr/backend.hpp>
#include <hicr/backends/sharedMemory/pthreads/sharedMemorySpace.hpp>
#include <hicr/backends/sharedMemory/pthreads/thread.hpp>

namespace HiCR
{

namespace backend
{

namespace sharedMemory
{

namespace pthreads
{

/**
 * Implementation of the Pthreads/HWloc-based HiCR Shared Memory Backend.
 *
 * It detects and returns the processing units and memory spaces detected by the HWLoc library, and instantiates the former as pthreads and the latter as MemorySpace descriptors. It also detects their connectivity.
 */
class Pthreads final : public Backend
{
  private:

  /**
   * list of deffered function calls in non-blocking data moves, which
   * complete in the wait call
   */
  std::multimap<Tag, std::future<void>> deferredFuncs;
  /**
   * Local processor and memory hierarchy topology, as detected by Hwloc
   */
  hwloc_topology_t _topology;

  public:

  Pthreads() = default;
  ~Pthreads() = default;

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

  /**
   * Pthread implementation of the Backend queryResources() function. This will add one resource object per found Thread / Processing Unit (PU)
   */
  __USED__ inline void queryResources() override
  {
    hwloc_topology_init(&_topology);
    hwloc_topology_load(_topology);

    std::vector<int> threadPUs;
    getThreadPUs(_topology, hwloc_get_root_obj(_topology), 0, threadPUs);

    // Creating Thread objects
    for (size_t i = 0; i < threadPUs.size(); i++)
    {
      auto affinity = std::vector<int>({threadPUs[i]});
      auto thread = std::make_unique<pthreads::Thread>(affinity);
      _computeResourceList.push_back(std::move(thread));
    }

    /* Ask hwloc about number of NUMA nodes
     * and add as many memory spaces as NUMA domains
     */
    auto n = hwloc_get_nbobjs_by_type(_topology, HWLOC_OBJ_NUMANODE);
    for (int i = 0; i < n; i++)
    {
      SharedMemorySpace *ms = new SharedMemorySpace(i, _topology);
      _memorySpaceList.push_back(ms);
    }
  }

  void nb_memcpy(
    MemorySlot &destination,
    const size_t dst_locality,
    const size_t dst_offset,
    const MemorySlot &source,
    const size_t src_locality,
    const size_t src_offset,
    const size_t size,
    const Tag &tag) override
  {
    std::function<void(void *, const void *, size_t)> f = [](void *dst, const void *src, size_t size)
    {
      std::memcpy(dst, src, size);
    };
    std::future<void> fut = std::async(std::launch::deferred, f, destination.getPointer(), source.getPointer(), size);
    deferredFuncs.insert(std::make_pair(tag, std::move(fut)));
  }

  void wait(const Tag &tag)
  {
    auto range = deferredFuncs.equal_range(tag);
    for (auto i = range.first; i != range.second; ++i)
    {
      auto f = std::move(i->second);
      f.wait();
    }
  }
};

} // namespace pthreads
} // namespace sharedMemory
} // namespace backend
} // namespace HiCR
