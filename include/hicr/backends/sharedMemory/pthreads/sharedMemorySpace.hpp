#pragma once

#include "hwloc.h"
#include <hicr/backends/sharedMemory/pthreads/sharedMemorySlot.hpp>

namespace HiCR
{
namespace backend
{
namespace sharedMemory
{
namespace pthreads
{

class SharedMemorySpace : public HiCR::MemorySpace
{
  private:

  const size_t _id;
  hwloc_topology_t _topology;

  public:

  SharedMemorySpace(size_t id, hwloc_topology_t topology) : _id(id), _topology(topology)
  {
  }
  ~SharedMemorySpace()
  {
  }

  size_t getID()
  {
    return _id;
  }

  hwloc_topology_t getTopology()
  {
    return _topology;
  }

  SharedMemorySlot allocateMemorySlot(
    const size_t size,
    const size_t myLocalityID = 0,
    SharedMemorySpace *remotes = nullptr,
    const SharedMemorySpace *remotes_end = nullptr)
  {
    SharedMemorySlot slot(size, getTopology(), getID());
    return slot;
  }
};
} // namespace pthreads
} // namespace sharedMemory
} // namespace backend
} // namespace HiCR
