/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file Provides various memcpy functions as well as corresponding fences.
 * @author A. N. Yzelman
 * @date 26/7/2023
 */

#pragma once

namespace HiCR {

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
 * If there is no direct path of communication possible between the memory
 * spaces that underlie \a source and \a destination, an exception will be
 * thrown.
 *
 * One (or both) of \a source and \a destination must be a global memory slot.
 *
 * A call to this function is one-sided, non-blocking, and, if the hardware and
 * network supports it, zero-copy.
 *
 * \note For blocking semantics, simply immediately follow this call to memcpy
 *       with a call to HiCR::fence().
 *
 * \todo Should this be <tt>nb_memcpy</tt> to make clear that, quite different
 *       from the NIX standard <tt>memcpy</tt>, it is nonblocking?
 *
 * \internal Since memory slots are tied to memory spaces, a sparse matrix
 *           \f$ M \f$ internal to HiCR prescribes which backends can facilitate
 *           data movement between pairs of memory spaces. This memcpy hence
 *           looks into this table \f$ M \f$, picks the right backend mechanism,
 *           and translates the memcpy call into that backend-specific call.
 *
 * Exceptions are thrown in the following cases:
 *  -# HiCR cannot facilitate communication between the requested memory spaces.
 *  -# The offset and sizes result in a communication request that is outside
 *     the region of either \a destination or \a source.
 *  -# Both \a destination and \a source are local memory slots.
 *  -# One (or both) of \a dst_locality and \a src_locality point to
 *     non-existing memory spaces.
 *  -# \a dst_locality is a local memory slot but \a dst_locality is not 0
 *  -# \a src_locality is a local memory slot but \a src_locality is not 0
 *
 * \todo Problem: currently, in fact, a MemorySpace is tied to a single address
 *       space, but the \a dst_locality and \a src_locality arguments indicate
 *       that the slot should be aware of remote connected memory spaces also.
 *       Is this another difference between a local and global memory slot?
 */
void memcpy(
 MemorySlot& destination, const size_t dst_locality, const size_t dst_offset,
 const MemorySlot& source, const size_t src_locality, const size_t src_offset,
 const size_t size,
 const TagSlot& tag
);

/**
 * Fences a group of memory copies.
 *
 * \warning This fence implies a (non-blocking) all-to-all across the entire
 *          system. While this latency can be hidden by yielding plus a
 *          completion event, the latency exists nonetheless. If, therefore,
 *          the latency cannot be hidden, then zero-cost fencing (see below)
 *          should be employed instead.
 *
 * \todo failure model
 */
void fence(const TagSlot& tag);

/**
 * Fences a group of memory copies using zero-cost synchronisation.
 *
 * This fence does \em not incur any all-to-all collective as part of the
 * synchronisation.
 *
 * @param[in] tag      The tag of the memory copies to fence for
 * @param[in] msgs_in  How many messages are incoming to this locality
 * @param[in] msgs_out How many messages are outgoing from this locality
 * @param[in] dests    A set of memory resources (localities) to which we
 *                     have outgoing memory copy requests
 * @param[in] sources  A set of memory resources (localities) from which
 *                     we have incoming memory copy requests
 *
 * \todo failure model
 */
void fence(
 const TagSlot& tag,
 const size_t msgs_in, const size_t msgs_out,
 const set<MemoryResource*>& dests,
 const set<MemoryResource*>& sources
);

} // end namespace HiCR

