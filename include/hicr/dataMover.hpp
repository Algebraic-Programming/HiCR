/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file dataMover.hpp
 * @brief Provides various memcpy functions as well as corresponding fences.
 * @author A. N. Yzelman
 * @date 26/7/2023
 */

#pragma once

#include <hicr/common/definitions.hpp>
#include <hicr/memorySlot.hpp>
#include <hicr/messageTag.hpp>

namespace HiCR
{

/**
 * Instructs the backend to perform an asynchronous memory copy from
 * within a source area, to within a destination area.
 *
 * @param[in] source       The source memory region
 * @param[in] src_locality The locality of the source memory region
 * @param[in] src_offset   The offset (in bytes) within \a source at
 *                         \a src_locality
 * @param[in] destination  The destination memory region
 * @param[in] dst_locality The locality of the destination memory region
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
 *       with a call any of the HiCR::fence() variants. If you would like a
 *       blocking memcpy, we can provide a small library that wraps this
 *       function with a fence. While this would perhaps be easier to use, it
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
HICR_API void nb_memcpy(
  MemorySlot &destination,
  const size_t dst_locality,
  const size_t dst_offset,
  const MemorySlot &source,
  const size_t src_locality,
  const size_t src_offset,
  const size_t size,
  const Tag &tag);

/**
 * Fences a group of memory copies.
 *
 * This is a collective and blocking call; returning from this function
 * indicates that all local incoming memory movement has completed \em and that
 * all outgoing memory movement has left the local interface (and is guaranteed
 * to arrive at the remote memory space, modulo any fatal exception).
 *
 * @param[in] tag The tag of the memory copies to fence for.
 *
 * \warning This fence implies a (non-blocking) all-to-all across all memory
 *          spaces the \a tag was created with. While this latency can be hidden
 *          by yielding plus a completion event or by polling (see test_fence),
 *          the latency exists nonetheless. If, therefore, this latency cannot
 *          be hidden, then zero-cost fencing (see below) should be employed
 *          instead.
 *
 * \note While the fence is blocking, local successful completion does \em not
 *       guarantee that all other memory spaces the given \a tag has been
 *       created with, are done with their local fence.
 *
 * Exceptions are thrown in the following cases:
 *  -# One of the remote address spaces no longer has an active communication
 *     channel. This is a fatal exception from which HiCR cannot recover. The
 *     user is encouraged to exit gracefully without initiating any further
 *     communication or fences.
 *
 * \todo How does this interact with malleability of resources of which HiCR is
 *       aware? One possible answer is a special event that if left unhandled,
 *       is promoted to a fatal exception.
 */
HICR_API void fence(const Tag &tag);

/**
 * Fences a group of memory copies using zero-cost synchronisation.
 *
 * This is a collective and blocking call; returning from this function
 * indicates that all local incoming memory movement has completed \em and that
 * all outgoing memory movement has left the local interface (and is guaranteed
 * to arrive at the remote memory space, modulo any fatal exception).
 *
 * This fence does \em not incur any all-to-all collective as part of the
 * operation; one way to look at this variant is that all meta-data an
 * all-to-all collects in the other fence variant here is provided as arguments
 * instead.
 *
 * @param[in] tag      The tag of the memory copies to fence for.
 * @param[in] msgs_out How many messages are outgoing from this locality.
 * @param[in] msgs_in  How many messages are incoming to this locality.
 * @param[in] dests    A stard iterator over localities to which we have outgoing
 *                     memory copy requests.
 * @param[in] dests_end  An end iterator over localities to which we have outgoing
 *                       memory copy requests.
 * @param[in] sources  A start iterator over localities from which we have incoming
 *                     memory copy requests.
 * @param[in] sources_end  An end iterator over localities from which we have incoming
 *                     memory copy requests.
 *
 * \note A remote worker may initiate a memory copy where the source is local
 *       to us. Even though the memory copy was not issued by us, this still
 *       counts as one outgoing message and should be accounted for by
 *       \a msgs_out and should lead to an entry iterated over by \a dests.
 *
 * \note A remote worker may initiate a memory copy where the destination is
 *       local to us. Even though the memory copy was not issued by us, this
 *       still counts as one incoming message and should be accounted for by
 *       \a msgs_in and should lead to an entry iterated over by \a sources.
 *
 * Exceptions are thrown in the following cases:
 *  -# An entry of \a dests or \a sources is out of range w.r.t. the localities
 *     \a tag has been created with.
 *  -# One of the remote address spaces no longer has an active communication
 *     channel. This is a critical failure from which HiCR cannot recover. The
 *     user is encouraged to exit gracefully without initiating any further
 *     communication or fences.
 *
 * \note One difference between the global fence is that this variant
 *       potentially may \em not detect remote failures of resources that are
 *       not tied to those in \a dests or \a sources; the execution may proceed
 *       without triggering any exception, until such point another fence waits
 *       for communication from a failed resource.
 *
 * \todo How does this interact with malleability of resources of which HiCR is
 *       aware? One possible answer is a special exception / event.
 */
template <typename DstIt = const size_t *, typename SrcIt = const size_t *>
void fence(
  const Tag &tag,
  const size_t msgs_out,
  const size_t msgs_in,
  DstIt dests,
  const DstIt dests_end,
  SrcIt sources,
  const SrcIt sources_end);

} // end namespace HiCR
