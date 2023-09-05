/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file backend.hpp
 * @brief Provides a definition for the backend class.
 * @author S. M. Martin
 * @date 27/6/2023
 */

#pragma once

#include <hicr/common/definitions.hpp>
#include <hicr/computeResource.hpp>

namespace HiCR
{

typedef uint64_t memorySpaceId_t;
typedef uint64_t memorySlotId_t;
typedef uint64_t tagId_t;

/**
 * Common definition of a collection of compute resources
 */
typedef std::vector<std::unique_ptr<ComputeResource>> computeResourceList_t;

/**
 * Common definition of a collection of memory spaces
 */

typedef std::vector<memorySlotId_t> memorySpaceList_t;

/**
 * Encapsulates a HiCR Backend.
 *
 * Backends represent plugins to HiCR that provide support for a communication or device library. By adding new plugins developers extend HiCR's support for new hardware and software technologies.
 *
 * Backends need to fulfill the abstract virtual functions described here, so that HiCR can perform common operations on the supported device/network library.
 *
 */
class Backend
{
  public:

  virtual ~Backend() = default;

  /**
   * This function prompts the backend to perform the necessary steps to discover and list the resources provided by the library which it supports.
   *
   * In case of change in resource availability during runtime, users need to re-run this function to be able to see the changes.
   *
   * \internal It does not return anything because we want to allow users to run only once, and then consult it many times without having to make a copy.
   */
  __USED__ virtual void queryResources() = 0;

  /**
   * This function returns the list of queried compute resources, as visible by the backend.
   *
   * If this function is called before queryResources, then it will return an empty container.
   *
   * @return The list of compute resources, as detected the last time \a queryResources was executed.
   */
  __USED__ inline computeResourceList_t &getComputeResourceList() { return _computeResourceList; }

  /**
   * This function returns the list of queried memory spaces, as visible by the backend.
   *
   * If this function is called before queryResources, then it will return an empty container.
   *
   * @return The list of memory spaces, as detected the last time \a queryResources was executed.
   */
  __USED__ inline memorySpaceList_t &getMemorySpaceList() { return _memorySpaceList; }

  /**
   * Instructs the backend to perform an asynchronous memory copy from
   * within a source area, to within a destination area.
   *
   * @param[in] source       The source memory region
   * @param[in] src_offset   The offset (in bytes) within \a source at
   *                         \a src_locality
   * @param[in] destination  The destination memory region
   * @param[in] dst_offset   The offset (in bytes) within \a destination at
   *                         \a dst_locality
   * @param[in] size         The number of bytes to copy from the source to the
   *                         destination
   * @param[in] tag          The tag of this memory copy
   *
   * A call to this function is one-sided, non-blocking, and, if the hardware and
   * network supports it, zero-copy.
   *
   * If there is no direct path of communication possible between the memory
   * spaces that underlie \a source and \a destination (and their localities), an
   * exception will be thrown.
   *
   * The memory spaces (localities) that \a source and \a destination have been
   * created with must exist within the memory spaces \a tag was created with.
   *
   * \note The reverse (the memory spaces \a tag was created with must exist
   *       within those that \a source or \a destination have been created with)
   *       need \em not be true; the set of memory spaces corresponding to the
   *       \a tag are a superset of the union of those corresponding to \a source
   *       and \a destination.
   *
   * \note If \a source is a local memory slot, then \a src_locality \em must be
   *       <tt>0</tt>-- a local memory slot only has its own locality.
   *
   * \note If \a destination is a local memory slot, then \a src_locality \em must
   *       be <tt>0</tt>-- a local memory slot only has its own locality.
   *
   * \note For blocking semantics, simply immediately follow this call to memcpy
   *       with a call any of the HiCR::wait() variants. If you would like a
   *       blocking memcpy, we can provide a small library that wraps this
   *       function with a wait. While this would perhaps be easier to use, it
   *       requires two-sided interaction (in case a remote memory space is
   *       involved) \em and will likely wreak havoc on performance of the upper-
   *       level run-time system.
   *
   * \internal Since memory slots are tied to memory spaces, a sparse matrix
   *           \f$ M \f$ internal to HiCR prescribes which backends can facilitate
   *           data movement between pairs of memory spaces. This memcpy hence
   *           looks into this table \f$ M \f$, picks the right backend mechanism,
   *           and translates the memcpy call into that backend-specific call.
   *
   * Exceptions are thrown in the following cases:
   *  -# HiCR cannot facilitate communication between the requested memory spaces;
   *  -# The offset and sizes result in a communication request that is outside
   *     the region of either \a destination or \a source;
   *  -# Both \a destination and \a source are local memory slots;
   *  -# One (or both) of \a dst_locality and \a src_locality point to
   *     non-existing memory spaces;
   *  -# \a dst_locality is a local memory slot but \a dst_locality is not 0;
   *  -# \a src_locality is a local memory slot but \a src_locality is not 0;
   *  -# the localities in \a tag are not a superset of the localities registered
   *     with \a source or \a destination.
   *
   * \todo Should this be <tt>nb_memcpy</tt> to make clear that, quite different
   *       from the NIX standard <tt>memcpy</tt>, it is nonblocking?
   */
  virtual void memcpy(
    memorySlotId_t destination,
    const size_t dst_offset,
    const memorySlotId_t source,
    const size_t src_offset,
    const size_t size,
    const tagId_t &tag) = 0;

  /**
   * Fences a group of memory copies.
   *
   * This is a collective and blocking call; returning from this function
   * indicates that all local incoming memory movement has completed \em and that
   * all outgoing memory movement has left the local interface (and is guaranteed
   * to arrive at the remote memory space, modulo any fatal exception).
   *
   * @param[in] tag The tag of the memory copies to wait for.
   *
   * \warning This wait implies a (non-blocking) all-to-all across all memory
   *          spaces the \a tag was created with. While this latency can be hidden
   *          by yielding plus a completion event or by polling (see test_wait),
   *          the latency exists nonetheless. If, therefore, this latency cannot
   *          be hidden, then zero-cost fencing (see below) should be employed
   *          instead.
   *
   * \note While the wait is blocking, local successful completion does \em not
   *       guarantee that all other memory spaces the given \a tag has been
   *       created with, are done with their local wait.
   *
   * Exceptions are thrown in the following cases:
   *  -# One of the remote address spaces no longer has an active communication
   *     channel. This is a fatal exception from which HiCR cannot recover. The
   *     user is encouraged to exit gracefully without initiating any further
   *     communication or waits.
   *
   * \todo How does this interact with malleability of resources of which HiCR is
   *       aware? One possible answer is a special event that if left unhandled,
   *       is promoted to a fatal exception.
   */
  virtual void fence(const tagId_t tag) = 0;

  /**
   * Allocates memory in the specified memory space
   *
   * \param[in] memorySpaceId Memory space to allocate memory in
   * \param[in] size Size of the memory slot to create
   * \return A newly allocated memory slot in the specified memory space
   */
  virtual memorySlotId_t allocateMemorySlot(const memorySpaceId_t memorySpaceId, const size_t size) = 0;

  /**
   * Creates a memory slot from a given address
   *
   * \param[in] addr Pointer to the start of the memory slot
   * \param[in] size Size of the memory slot to create
   * \return A newly created memory slot
   */
  virtual memorySlotId_t createMemorySlot(void *const addr, const size_t size) = 0;

  virtual void* getMemorySlotLocalPointer(const memorySlotId_t slot) const = 0;
  virtual size_t getMemorySlotSize(const memorySlotId_t slot) const = 0;

  /**
   * This function prompts the backend to perform the necessary steps to discover and list the resources provided by the library which it supports.
   *
   * In case of change in resource availability during runtime, users need to re-run this function to be able to see the changes.
   *
   * \internal It does not return anything because we want to allow users to run only once, and then consult it many times without having to make a copy.
   */
  /**
   * The internal container for the queried resources.
   */
  computeResourceList_t _computeResourceList;

  /**
   * The internal container for the queried resources.
   */
  memorySpaceList_t _memorySpaceList;
};

} // namespace HiCR
