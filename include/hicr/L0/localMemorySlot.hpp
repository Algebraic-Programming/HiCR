/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file localMemorySlot.hpp
 * @brief Provides a definition for a HiCR Local Memory Slot class
 * @author S. M. Martin
 * @date 4/10/2023
 */
#pragma once

#include <hicr/definitions.hpp>
#include <hicr/L0/memorySpace.hpp>

namespace HiCR
{

namespace L0
{

/**
 * This class represents an abstract definition for a Local Memory Slot resource in HiCR that:
 *
 * - Represents a contiguous segment within a memory space in the local system, with a starting address and a size
 */
class LocalMemorySlot
{
  public:

  /**
   * Default constructor for a LocalMemorySlot class
   *
   * \param[in] pointer The pointer corresponding to an address in a given memory space
   * \param[in] size The size (in bytes) of the memory slot, assumed to be contiguous
   * \param[in] memorySpace Pointer to the memory space that this memory slot belongs to. NULL, if the memory slot is global (remote)
   */
  LocalMemorySlot(
    void *const pointer,
    const size_t size,
    HiCR::L0::MemorySpace *memorySpace = NULL) : _pointer(pointer),
                                                 _size(size),
                                                 _memorySpace(memorySpace)
  {
  }

  /**
   * Default destructor
   */
  virtual ~LocalMemorySlot() = default;

  /**
   * Getter function for the memory slot's pointer
   * \returns The memory slot's internal pointer
   */
  __USED__ inline void *getPointer() const noexcept { return _pointer; }

  /**
   * Getter function for the memory slot's size
   * \returns The memory slot's size
   */
  __USED__ inline size_t getSize() const noexcept { return _size; }

  /**
   * Getter function for the memory slot's associated memory space
   * \returns The memory slot's associated memory space
   */
  __USED__ inline HiCR::L0::MemorySpace *getMemorySpace() const noexcept { return _memorySpace; }

  private:

  /**
   * Pointer to the local memory address containing this slot
   */
  void *const _pointer;

  /**
   * Size of the memory slot
   */
  const size_t _size;

  /**
   * Memory space that this memory slot belongs to
   */
  L0::MemorySpace *const _memorySpace;
};

} // namespace L0

} // namespace HiCR
