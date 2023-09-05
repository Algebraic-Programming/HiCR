#pragma once

#include "hwloc.h"
#include <hicr/common/definitions.hpp>
#include <hicr/memorySlot.hpp>

namespace HiCR
{
namespace backend
{
namespace sharedMemory
{
namespace pthreads
{

/**
 * Implementation of a memory space class for the Pthreads backend
 */
class SharedMemorySpace : public HiCR::MemorySpace
{
  private:

  const size_t _id;
  hwloc_topology_t _topology;

  public:

  /**
   * Constructor for the Shared Memory space class, allowing the user to specify an HWLoC topology associated to it
   *
   * \param[in] id Unique local identifier for the space
   * \param[in] topology HWLoC topology that exposes how the NUMA domains are indexed
   */
  SharedMemorySpace(size_t id, hwloc_topology_t topology) : _id(id), _topology(topology) {}

  /**
   * Returns the memory space ID
   *
   * \return Unique local identifier for the space
   */
  __USED__ inline size_t getID() const
  {
    return _id;
  }

  /**
   * Allocates memory in the current memory space (NUMA domain)
   *
   * \param[in] size Size of the memory slot to create
   * \return A newly allocated memory slot in this memory space
   */
  __USED__ inline MemorySlot allocateMemorySlot(const size_t size) const
  {
    hwloc_obj_t obj = hwloc_get_obj_by_type(_topology, HWLOC_OBJ_NUMANODE, _id);
    ptr_t memSlotPointer = hwloc_alloc_membind(_topology, size, obj->nodeset, HWLOC_MEMBIND_BIND, HWLOC_MEMBIND_BYNODESET);

    return MemorySlot(memSlotPointer, size);
  }

  /**
   * Frees up a memory slot reserved from this memory space
   *
   * \param[in] slot Pointer to a memory slot to free up. It becomes unusable after freeing.
   */
  __USED__ inline void freeMemorySlot(MemorySlot *slot) const
  {
    hwloc_free(_topology, slot->getPointer(), slot->getSize());
  }
};

} // namespace pthreads
} // namespace sharedMemory
} // namespace backend
} // namespace HiCR
