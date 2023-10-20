/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file memorySlot.hpp
 * @brief Provides a definition for the memory slot class for the shared Memory backend
 * @author S. M. Martin
 * @date 19/10/2023
 */
#pragma once

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

  __USED__ inline binding_type getBindingType() { return _bindingType; }

  private:

  /**
   * Store whether a bound memory allocation has performed
   */
  binding_type _bindingType;
};

} // namespace sharedMemory

} // namespace backend

} // namespace HiCR
