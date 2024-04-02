/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file localMemorySlot.hpp
 * @brief Provides a definition for the local memory slot class for the HWLoc-based backend
 * @author S. M. Martin
 * @date 19/10/2023
 */
#pragma once

#include <hicr/core/L0/localMemorySlot.hpp>

namespace HiCR
{

namespace backend
{

namespace host
{

namespace hwloc
{

namespace L0
{

/**
 * This class represents an abstract definition for a Memory Slot resource in HiCR that:
 *
 * - Represents a contiguous segment within a memory space, with a starting address and a size
 */
class LocalMemorySlot final : public HiCR::L0::LocalMemorySlot
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
    strict_binding = 2,

    /**
     * With strict non-binding, the memory is given by the system allocator. In this case, the binding is most likely setup by the first thread that touches the reserved pages (first touch policy)
     */
    strict_non_binding = 1,

    /**
     * With relaxed binding, the memory manager attempts to allocate the memory with a binding, but defaults to non-binding if not supported
     */
    relaxed_binding = 0
  };

  /**
   * Constructor for a MemorySlot class for the MPI backend
   *
   * \param[in] bindingType The binding type requested (and employed) for this memory slot
   * \param[in] pointer If this is a local slot (same rank as this the running process), this pointer indicates the address of the local memory segment
   * \param[in] size The size (in bytes) of the memory slot, assumed to be contiguous
   * \param[in] memorySpace The memory space from whence this memory slot was created
   */
  LocalMemorySlot(binding_type bindingType, void *const pointer, const size_t size, std::shared_ptr<HiCR::L0::MemorySpace> memorySpace)
    : HiCR::L0::LocalMemorySlot(pointer, size, memorySpace),
      _bindingType(bindingType)
  {}

  ~LocalMemorySlot() = default;

  /**
   * Returns the binding type used to allocate/register this memory slot
   *
   * \return The binding type used to allocate/register this memory slot
   */
  __INLINE__ binding_type getBindingType() const { return _bindingType; }

  private:

  /**
   * Store whether a bound memory allocation has performed
   */
  const binding_type _bindingType;
};

} // namespace L0

} // namespace hwloc

} // namespace host

} // namespace backend

} // namespace HiCR
