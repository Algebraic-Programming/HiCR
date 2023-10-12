/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file dataMover.hpp
 * @brief Provides a definition for the base backend data mover class.
 * @author S. M. Martin
 * @date 12/10/2023
 */

#pragma once

#include <hicr/common/definitions.hpp>
#include <hicr/common/exceptions.hpp>
#include <hicr/memorySlot.hpp>

namespace HiCR
{

namespace backend
{

/**
 * Encapsulates a HiCR Backend.
 *
 * Backends represent plugins to HiCR that provide support for a communication or device library. By adding new plugins developers extend HiCR's support for new hardware and software technologies.
 *
 * Backends need to fulfill the abstract virtual functions described here, so that HiCR can perform common operations on the supported device/network library.
 *
 */
class DataMover
{
  public:

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
   *
   * A call to this function is one-sided, non-blocking, and, if the hardware and
   * network supports it, zero-copy.
   *
   * If there is no direct path of communication possible between the memory
   * spaces that underlie \a source and \a destination (and their localities), an
   * exception will be thrown.
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
   *
   * \todo Should this be <tt>nb_memcpy</tt> to make clear that, quite different
   *       from the NIX standard <tt>memcpy</tt>, it is nonblocking?
   */
  __USED__ inline void memcpy(MemorySlot *destination, const size_t dst_offset, MemorySlot *source, const size_t src_offset, const size_t size)
  {
    // Getting slot sizes. This operation is thread-safe
    const auto srcSize = source->getSize();
    const auto dstSize = destination->getSize();

    // Making sure the memory slots exist and is not null
    const auto actualSrcSize = size + src_offset;
    const auto actualDstSize = size + dst_offset;

    // Checking size doesn't exceed slot size
    if (actualSrcSize > srcSize) HICR_THROW_RUNTIME("Memcpy size (%lu) + offset (%lu) = (%lu) exceeds source slot (%lu) capacity (%lu).", size, src_offset, actualSrcSize, source, srcSize);

    // Checking size doesn't exceed slot size
    if (actualDstSize > dstSize) HICR_THROW_RUNTIME("Memcpy size (%lu) + offset (%lu) = (%lu) exceeds destination slot (%lu) capacity (%lu).", size, dst_offset, actualDstSize, destination, dstSize);

    // To enable concurrent memcpy operations, the implementation is executed outside the mutex zone
    // This means that the developer needs to make sure that the implementation is concurrency-safe,
    // and try not to access any of the internal Backend class fields without proper mutex locking

    // Now calling internal memcpy function to give us a function that satisfies the operation
    memcpyImpl(destination, dst_offset, source, src_offset, size);
  }

  /**
   * Fences a group of memory copies.
   *
   * This is a collective and blocking call; returning from this function
   * indicates that all local incoming memory movement has completed \em and that
   * all outgoing memory movement has left the local interface (and is guaranteed
   * to arrive at the remote memory space, modulo any fatal exception).
   *
   * This function also finishes all pending local to global memory slot promotions,
   * only for the specified tag.
   *
   * \param[in] tag A tag that releases all processes that share the same value once they have arrived at it
   * Exceptions are thrown in the following cases:
   *  -# One of the remote address spaces no longer has an active communication
   *     channel. This is a fatal exception from which HiCR cannot recover. The
   *     user is encouraged to exit gracefully without initiating any further
   *     communication or waits.
   *
   * \todo How does this interact with malleability of resources of which HiCR is
   *       aware? One possible answer is a special event that if left unhandled,
   *       is promoted to a fatal exception.
   *
   * \todo This all should be threading safe.
   */
  __USED__ inline void fence(const tag_t tag)
  {
    // To enable concurrent fence operations, the implementation is executed outside the mutex zone
    // This means that the developer needs to make sure that the implementation is concurrency-safe,
    // and try not to access any of the internal Backend class fields without proper mutex locking

    // Now call the proper fence, as implemented by the backend
    fenceImpl(tag);
  }

  protected:

  /**
   * Backend-internal implementation of the memcpy function
   *
   * @param[in] source       The source memory region
   * @param[in] src_offset   The offset (in bytes) within \a source at \a src_locality
   * @param[in] destination  The destination memory region
   * @param[in] dst_offset   The offset (in bytes) within \a destination at \a dst_locality
   * @param[in] size         The number of bytes to copy from the source to the destination
   */
  virtual void memcpyImpl(MemorySlot *destination, const size_t dst_offset, MemorySlot *source, const size_t src_offset, const size_t size) = 0;

  /**
   * Backend-internal implementation of the fence function
   *
   * \param[in] tag A tag that releases all processes that share the same value once they have arrived at it
   *
   */
  virtual void fenceImpl(const tag_t tag) = 0;

};

} // namespace backend

} // namespace HiCR
