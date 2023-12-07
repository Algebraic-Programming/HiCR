/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file memorySlot.hpp
 * @brief Provides a definition for the memory slot class for the shared memory backend
 * @author S. M. Martin
 * @date 19/10/2023
 */
#pragma once

#include <hicr/L0/memorySlot.hpp>

namespace HiCR
{

namespace backend
{

namespace sharedMemory
{

/**
 * This class represents an abstract definition for a Memory Slot resource in HiCR that:
 *
 * - Represents a contiguous segment within a memory space, with a starting address and a size
 */
class MemorySlot final : public HiCR::L0::MemorySlot
{
  public:

  /**
   * Enumeration to determine whether HWLoc supports strict binding and what the user prefers (similar to MPI_Threading_level)
   */
  enum binding_type
  {
    /**
     * With strict binding, the memory is allocated strictly in the specified memory space
     */
    strict_binding = 1,

    /**
     * With strict non-binding, the memory is given by the system allocator. In this case, the binding is most likely setup by the first thread that touches the reserved pages (first touch policy)
     */
    strict_non_binding = 0
  };

  /**
   * Constructor for a MemorySlot class for the MPI backend
   *
   * \param[in] bindingType The binding type requested (and employed) for this memory slot
   * \param[in] pointer If this is a local slot (same rank as this the running process), this pointer indicates the address of the local memory segment
   * \param[in] size The size (in bytes) of the memory slot, assumed to be contiguous
   * \param[in] globalTag For global memory slots, indicates the subset of global memory slots this belongs to
   * \param[in] globalKey Unique identifier for that memory slot that this slot occupies.
   */
  MemorySlot(
    binding_type bindingType,
    void *const pointer,
    const size_t size,
    const HiCR::L0::MemorySlot::tag_t globalTag = 0,
    const HiCR::L0::MemorySlot::globalKey_t globalKey = 0) : HiCR::L0::MemorySlot(pointer, size, globalTag, globalKey),
                                                            _bindingType(bindingType)
  {
    pthread_mutex_init(&_mutex, NULL);
  }

  ~MemorySlot()
  {
    // Freeing mutex memory
    pthread_mutex_destroy(&_mutex);
  }

  /**
   * Returns the binding type used to allocate/register this memory slot
   *
   * \return The binding type used to allocate/register this memory slot
   */
  __USED__ inline binding_type getBindingType() const { return _bindingType; }

  /**
   * Attempts to lock memory lock using its pthread mutex object
   *
   * This function never blocks the caller
   *
   * @return True, if successful; false, otherwise.
   */
  __USED__ inline bool trylock() { return pthread_mutex_trylock(&_mutex) == 0; }

  /**
   * Attempts to lock memory lock using its pthread mutex object
   *
   * This function might block the caller if the memory slot is already locked
   */
  __USED__ inline void lock() { pthread_mutex_lock(&_mutex); }

  /**
   * Unlocks the memory slot, if previously locked by the caller
   */
  __USED__ inline void unlock() { pthread_mutex_unlock(&_mutex); }

  private:

  /**
   * Store whether a bound memory allocation has performed
   */
  binding_type _bindingType;

  /**
   * Internal memory slot mutex to enforce lock acquisition
   */
  pthread_mutex_t _mutex;
};

} // namespace sharedMemory

} // namespace backend

} // namespace HiCR
