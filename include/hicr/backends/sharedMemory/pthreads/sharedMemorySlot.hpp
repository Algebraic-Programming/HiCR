/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file memorySlot.hpp
 * @brief Provides a definition for the memory slot class.
 * @author A. N. Yzelman
 * @date 8/8/2023
 */

#pragma once

#include "hwloc.h"
#include <hicr/memorySlot.hpp>

namespace HiCR
{
namespace backend
{
namespace sharedMemory
{
namespace pthreads
{

class SharedMemorySlot : public HiCR::MemorySlot
{
  private:

  size_t _size;
  hwloc_topology_t _topology;

  public:

  SharedMemorySlot(size_t size, hwloc_topology_t topology, size_t numaDomain) : MemorySlot(setBuffer(numaDomain, size, topology))
  {
    _topology = topology;
    _size = size;
    if (getPointer() == NULL)
    {
      std::cerr << "Failed to allocate memory via hwloc_alloc_membind\n";
      std::abort();
    }
  }

  ~SharedMemorySlot()
  {
    hwloc_free(_topology, getPointer(), _size);
  }

  ptr_t setBuffer(size_t numaDomain, size_t size, hwloc_topology_t topology)
  {
    hwloc_obj_t obj = hwloc_get_obj_by_type(topology, HWLOC_OBJ_NUMANODE, numaDomain);
    ptr_t retValue = hwloc_alloc_membind(topology, size, obj->nodeset, HWLOC_MEMBIND_BIND, HWLOC_MEMBIND_BYNODESET);
    return retValue;
  }
};

} // end namespace pthreads
} // end namespace sharedMemory
} // end namespace backend
} // end namespace HiCR
