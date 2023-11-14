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

#include <mutex>
#include <hicr/memorySlot.hpp>

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
class MemorySlot final : public HiCR::MemorySlot
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
    const tag_t globalTag = 0,
    const globalKey_t globalKey = 0) : HiCR::MemorySlot(pointer, size, globalTag, globalKey),
                                       _bindingType(bindingType)
  {
  }

  ~MemorySlot() = default;

  /**
   * Returns the binding type used to allocate/register this memory slot
   *
   * \return The binding type used to allocate/register this memory slot
   */
  __USED__ inline binding_type getBindingType() const { return _bindingType; }

  __USED__ inline bool trylock() { return _mutex.try_lock(); }
  __USED__ inline void lock() { _mutex.lock(); }
  __USED__ inline void unlock() { _mutex.unlock(); }

  private:

  /**
   * Store whether a bound memory allocation has performed
   */
  binding_type _bindingType;

  std::mutex _mutex;
};

} // namespace sharedMemory

} // namespace backend

} // namespace HiCR
