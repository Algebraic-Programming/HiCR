/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file memorySpace.hpp
 * @brief Provides a definition for the memory space class.
 * @author A. N. Yzelman
 * @date 8/8/2023
 */

#pragma once

#include <hicr/channel.hpp>
#include <hicr/common/definitions.hpp>
#include <hicr/memorySlot.hpp>
#include <hicr/messageTag.hpp>

namespace HiCR
{

/**
 * This is a specialisation of the Resource class in HiCR, meant to express a
 * hardware memory element within a hierarchy (E.g., Cache, RAM, HBM, Device)
 *
 * \todo There is currently no abstract Resource type that is not intended as
 *       a compute resource. Therefore `specialisation' in the above text
 *       seeems incorrect-- there is currently just two classes:
 *         -# Resource, which encapsulates a compute resource, and
 *         -# MemorySpace, which encapsulates a memory resource (and more
 *            specifically, and at current at least, a buffer memory).
 *
 * At present, HiCR will only deal with RAM and device memory. Each compute
 * resource has associated with it a MemorySpace in which it resides. In its
 * most broad representation, it corresponds to any single address space
 * exposed by a system.
 *
 * Memory spaces are exposed by backends. A memory space can return memory
 * slots that indicate some memory region within said space. Unlike memory
 * spaces, which are always exposed by a single backend, memory slots could
 * interact with multiple backends; for example, a host memory region could
 * serve as * source or destination for memory copies to and from accelerators
 * or other remote nodes, thus requiring interaction with, again e.g., an ACL
 * backend or an ibverbs backend.
 *
 * Memory slots can either be created via allocation or via registration of
 * (user) memory. In the former case, releasing the memory slot will deallocate
 * its memory, while in the latter case deallocation remains the responsibility
 * of the user.
 *
 * There is a many-to-one relation between compute resources and memory spaces.
 * HiCR thus exposes memory spaces via getter functions to any compute resource.
 *
 * In future, there should be two broad classes of MemorySpaces: caches and
 * buffers. The current API in essence assumes buffers; i.e., managed memory.
 *
 * The interface for caches do not allow for the derivation of memory slots.
 * Instead, they allow for abstract operations such as flush, prefetch, or
 * invalidate. Backends may or may not support such operations and may do so at
 * different degrees of accuracy -- depending also on the underlying hardware.
 */
class MemorySpace final
{
  private:

  const size_t _id;

  public:

  MemorySpace(const size_t id) : _id(id) {};

  /**
   * Returns the memory space ID
   *
   * \return Unique local identifier for the space
   */
  __USED__ inline size_t getID() const
  {
    return _id;
  }

  /**
   * Constructs a channel.
   *
   * @tparam SrcIt An iterator over HiCR MemorySpaces
   * @tparam DstIt An iterator over HiCR MemorySpaces
   *
   * Let \f$ S \f$ be a set of producers, and let \f$ D \f$ be a set of
   * consumers. \f$ S, D \f$ must contain at least one element. A channel lets
   * any producer put so-called \em tokens into a distributed buffer, and lets
   * any consumer retrieve tokens from that buffer.
   *
   * A channel is identified by a \a tag, and as such, it makes use of system
   * resources equivalent to a single call to createTag.
   *
   * In addition, the channel requires \f$ n = |S| + |D| \f$ buffers, and (thus)
   * as many memory slots. Hence the channel, on successful creation, makes use
   * of system resources equivalent to \f$ n \f$ calls to allocateMemorySlot.
   *
   * The buffers and resources the channel allocates on successful construction,
   * will be released on a call to the channel destructor.
   *
   * @param[in] producer        Whether the calling context expects a producer
   *                            ChannelView. If not, it is assumed to expect a
   *                            consumer ChannelView.
   * @param[in] producers_start An iterator in begin position to \f$ S \f$
   * @param[in] producers_end   An iterator in end position to \f$ S \f$
   * @param[in] consumers_start An iterator in begin position to \f$ D \f$
   * @param[in] consumers_end   An iterator in end position to \f$ D \f$
   * @param[in] capacity        How many tokens may be in the channel at
   *                            maximum, at any given time
   * @param[in] dummy           Needs to be clarified
   *
   * A call to this constructor must be made collectively across all workers
   * that house the given memory spaces. If the callee memory space is in
   * \f$ S \f$ but not in \f$ D \f$, then the constructed channel must be a
   * \em producer. If the callee locality is in \f$ D \f$ but not in \f$ S \f$,
   * then the constructed channel must be a \em consumer. If there are duplicate
   * memory spaces in \f$ S \cup D \f$, then equally many calls to #createChannel
   * from each duplicate memory space are required. The current memory space must
   * be at least in one of \f$ S \f$ or \f$ D \f$. Finally, \f$ n \f$ must be
   * strictly larger than one.
   *
   * \note \f$ n > 1 \f$ still allows for a channel with sources and destinations
   *       wholle contained within a single memory space.
   *
   * With normal semantics, a produced token ends up at all (potentially many)
   * consumers. This channel, however, includes a possibility where tokens once
   * submitted are sent to just one of the consumers.
   *
   * @param[in] producersBroadcast Whether submitted tokens are to be broadcast
   *                               to all consumers. Optional; default is
   *                               <tt>true</tt>.
   *
   * @return Returns a view object to the newly created channel
   *
   * In broadcasting mode, broadcasting any single token to \f$ c = |D| \f$
   * consumers counts as taking up \f$ c \f$ \a capacity.
   *
   * \note Rationale: in the worst case, the token ends up at \f$ c \f$
   *       receiving buffers without any one of them being consumed yet.
   *
   * Channels always encapsulate one-copy communication. This means there is
   * always at least one copy of a token in either a sender or receiver buffer.
   *
   * \note For zero-copy communication mechanisms, see HiCR::memcpy.
   *
   * This function may fail for the following reasons:
   *  -# the current memory space is not in \f$ S \f$ nor \f$ D \f$;
   *  -# \a producer is <tt>false</tt> but the current memory space is not in
   *     \f$ D \f$;
   *  -# \a producer is <tt>true</tt> but the current memory space is not in
   *     \f$ S \f$;
   *  -# \f$ n \f$ equals one or at least one of \f$ c, p \f$ equals zero, with
   *     \f$ p = |S| \f$;
   *  -# the HiCR communication matrix \f$ M \f$ indicates a pair of memory spaces
   *     in \f$ S \cup D \f$ that have no direct line of communication;
   *  -# the order of elements given by \a producers_start differs across the
   *     collective call;
   *  -# the order of elements given by \a consumers_start differs across the
   *     collective call;
   *  -# a related backend is out of resources to create this new channel.
   *
   * @see HiCR::memcpy for a definition of \f$ M \f$.
   *
   * \todo This interface uses iterators instead of raw arrays for listing the
   *       producers and consumers. The memorySpace and datamover interfaces use
   *       raw arrays instead. We should probably just pick one API and make all
   *       APIs consistent with that choice.
   */
  template <typename SrcIt, typename DstIt, typename T>
  ChannelView<T> createChannel(
    bool producer,
    const SrcIt &producers_start,
    const SrcIt &producers_end,
    const DstIt &consumers_start,
    const DstIt &consumers_end,
    const size_t capacity,
    const bool producersBroadcast = true,
    const T &dummy = T());

  /*
   * This operation will resolve the release of the allocated memory space which this slot holds.
   * \todo NOTE (AJ) disabled since I didn't get why this should not be embedded in the slot destructor
   void MemoryResource::freeMemorySlot();
   */
};

} // end namespace HiCR
