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

#include <hicr/common/definitions.hpp>

namespace HiCR
{

/**
 * Typedef for the base class buffer -- it might not be
 * appropriate for some special memory slots.
 */
typedef void *const ptr_t;

/**
 * Encapsulates a memory region that is either the source or destination of a
 * call to put/get operations.
 *
 * Each backend provides its own type of MemorySlot.
 *
 * There might be limited number of memory slots supported by a given backend.
 * When a memory slot goes out of scope (or otherwise has its destructor
 * called), any associated resources are immediately freed. If the HiCR
 * MemorySpace class was responsible for allocating the underlying memory, then
 * that memory shall be freed also.
 *
 * \note This means that destroying a memory slot would immediately allow
 *       creating a new one, without running into resource constraints.
 */
class MemorySlot
{
  protected:

  MemorySlot();

  private:

  /**
   * Internal pointer for the memory slot buffer
   */
  ptr_t _buffer;

  /**
   * Size of the slot allocation
   */
  size_t _size;

  public:

  /**
   * A non-default constructor setting the generic buffer pointer
   *
   * @param[in] buffer A generic buffer pointer
   * @param[in] size Size of the memory slot (in bytes)
   */
  MemorySlot(ptr_t buffer, size_t size) : _buffer(buffer), _size(size)
  {
  }

  /**
   * Releases all resources associated with this memory slot. If the memory
   * slot was created via a call to MemorySpace::allocateMemorySlot, then the
   * associated memory will be freed as part of destruction.
   *
   * \internal In OO programming, standard practice is to declare destructors
   *           virtual.
   */
  virtual ~MemorySlot() {}

  /**
   * @returns The address of the memory region the slot has registered.
   *
   * This function when called on a valid MemorySlot instance may not fail.
   *
   * \todo This interface assumes any backend equates a memory slot to a
   */
  __USED__ ptr_t &getPointer() const noexcept
  {
    return _buffer;
  }

  /**
   * @returns The size of the memory region the slot has registered.
   *
   * This function when called on a valid MemorySlot instance may not fail.
   */
  __USED__ inline size_t getSize() const noexcept
  {
    return _size;
  }

  /**
   * @returns The number of localities this memorySlot has been created with.
   *
   * This function never returns <tt>0</tt>. When given a valid MemorySlot
   * instance, a call to this function never fails.
   *
   * \note When <tt>1</tt> is returned, this memory slot is a local slot. If
   *       a larger value is returned, the memory slot is a global slot.
   *
   * When referring to localities corresponding to this memory slot, only IDs
   * strictly lower than the returned value are valid.
   */
  __USED__ inline size_t nLocalities() const noexcept;
};

} // end namespace HiCR
