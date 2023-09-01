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
class MemorySpace
{
  protected:

  /**
   * as a memory space creates tags, it
   *needs to also ID them, e.g. via a counter
   */
  size_t _tagCounter = 0;

  /**
   * A memory space cannot be constructed -- it may only be retrieved from a
   * Resource instance.
   */
  // MemorySpace(); <= Kiril: I commented this as I get compile issues otherwise

  public:

  /**
   * \internal In OO programming, standard practice is to declare destructors
   *           virtual. There is no user-visible effect of destroying a memory
   *           space instance.
   */
  virtual ~MemorySpace() {}

  /**
   * Allocates a new memory slot within the memory resource.
   *
   * \note Explicitly acquiring a memory slot is required since some backends have
   *       limited memory slots, such as e.g. an Infiniband NIC which requires
   *       hardware buffers for each slot.
   *
   * @param[in] size  The size of the memory region to be registered.
   *
   * A memory slot may potentially need to be registered with backends other than
   * the backend that corresponds to this memory space. To facilitate this, the
   * \a interactsWith argument is a list of other memory spaces beyond the current
   * one that the slot may be used with. By default, the list is empty, indicating
   * that the retrieved memory slot is to only facilitate memory copies within its
   * own memory space.
   *
   * A memory slot has a concept of \em localities, which map one-to-one to memory
   * spaces. Every memory slot has at least one locality that corresponds to this
   * memory space. A locality is referenced by a unique <tt>size_t</tt> in the
   * range of 0 to \f$ n \f$ (exclusive), where \f$ n \f$ is the total number of
   * localities to be associated with the requested memory slot.
   *
   * @param[in] myLocalityID The ID of my locality.
   *
   * @param[in] remotes     An iterator over memory spaces that the memory slot
   *                        may interact with. Optional; by default, this is an
   *                        empty array (<tt>nullptr</tt>)
   * @param[in] remotes_end An iterator in end position that matches \a remotes.
   *                        Optional; by default, this is equal to \a remotes
   *                        (i.e., signifying an empty set of memory spaces).
   * @return Returns an interator to a memory slot collection
   *
   * The value for \a myLocalityID must be less than or equal to the number of
   * elements in \a remotes. Every \f$ i \f$-th entry of \a remotes that is larger
   * or equal to \a myLocalityID will have ID \f$ i + 1 \f$; all other entries of
   * \a remotes will have ID \f$ i \f$.
   *
   * When \a remotes are empty, a \em local memory slot will be returned. A local
   * memory slot can only be used within this MemorySpace, and can only be used to
   * refer to local source regions or local destination regions.
   *
   * A memory slot that is not local is called a \em global memory slot instead.
   * A global memory slot has \$f k = n + 1 \$ \em localities. Unlike a local
   * memory slot, a global memory slot \em can be used to refer to \em remote
   * source regions or remote destination regions; i.e., global memory slot can be
   * used to request data movement between different memory spaces.
   *
   * If there are no direct paths of communication between any pair in the set of
   * current memory space and the memory spaces in \a remotes, an exception will
   * be thrown.
   *
   * \warning Use of a returned memory slot within a memory copy that has as
   *          source or destination a memory slot that is not owned by this memory
   *          space, while that memory space was not given as part of
   *          \a remotes, invites undefined behaviour.
   *
   * When creating a global memory slot, the call to this primitive must be
   * \em collective across all workers / memory spaces in \a remotes. The order
   * in which remote memory spaces are iterated over furthermore must be equal
   * across all collective calls.
   *
   * This function may fail for the following reasons:
   *  -# out of memory;
   *  -# a related backend is out of resources to create a new memory slot;
   *  -# there is a duplicate memory space in the \a remotes array;
   *  -# if there is no direct path of communication possible between any pair in
   *     the set of the current memory space and all memory spaces in \a remotes;
   *  -# the order of memory spaces (if any) in \a remotes differs across the
   *     collective call.
   *
   * @see createMemorySlot The allocateMemorySlot may be viewed as combining a
   *      malloc with a call to #createMemorySlot. It is useful for buffers
   *      managed by the run-time system itself; neither function is designed
   *      to be called by the run-time end-user.
   */
  template <typename FwdIt = MemorySpace *>
  MemorySlot allocateMemorySlot(
    const size_t size,
    const size_t myLocalityID = 0,
    FwdIt remotes = nullptr,
    const FwdIt remotes_end = nullptr);

  /**
   * Creates a memory slot within this memory resource and given an existing
   * allocation.
   *
   * \note Explicitly acquiring a memory slot is required since some backends have
   *       limited memory slots, such as e.g. an Infiniband NIC which requires
   *       hardware buffers for each slot.
   *
   * @param[in] addr  The start address of the memory region to be registered.
   * @param[in] size  The size of the memory region to be registered.
   *
   * @param[in] myLocalityID The ID of my locality. Optional; the default assumes
   *                         a local memory slot and reads zero.
   * @param[in] remotes     An iterator over memory spaces that the memory slot
   *                        may interact with. Optional; by default, this is an
   *                        empty array (<tt>nullptr</tt>)
   * @param[in] remotes_end An iterator in end position that matches \a remotes.
   *                        Optional; by default, this is equal to \a remotes
   *                        (i.e., signifying an empty set of memory spaces).
   * @return Returns a the newly created memory slot
   *
   * @see allocateMemorySlot for more details regarding \a remotes and locality
   *      IDs. In particular, note that an empty array \a remotes on a successful
   *      call results in a local memory slot being returned, while a non-empty
   *      \a remotes results in a global memory slot being returned.
   *
   * When creating a global memory slot, the call to this primitive must be
   * \em collective across all workers / memory spaces in \a remotes. The order
   * in which remote memory spaces are iterated over furthermore must be equal
   * across all collective calls.
   *
   * Destroying a returned memory slot will \em not free the memory at \a addr.
   *
   * This function may fail for the following reasons:
   *  -# a related backend is out of resources to create a new memory slot;
   *  -# there is a duplicate memory space in the \a remotes array;
   *  -# if there is no direct path of communication possible between any pair in
   *     the set of the current memory space and all memory spaces in \a remotes;
   *  -# the order of memory spaces (if any) in \a remotes differs across the
   *     collective call.
   *
   * @see allocateMemorySlot The #createMemorySlot may be viewed as a variant of
   *      #allocateMemorySlot that skips allocation and instead registers a given
   *      already-allocated memory region. This function is to be used by a
   *      run-time for registering memory areas managed by the run-time end user;
   *      neither #createMemorySlot nor #allocateMemorySlot are designed to be
   *      called by the run-time end user.
   */
  template <typename FwdIt = MemorySpace *>
  MemorySlot createMemorySlot(
    void *const addr,
    const size_t size,
    const size_t myLocalityID,
    FwdIt remotes = nullptr,
    const FwdIt remotes_end = nullptr);

  /**
   * Creates a tag slot for use with memory slots that are created via calls to
   * #allocateMemorySlot or #createMemorySlot.
   *
   * Like memory slots, tags have a set of localities. The memory spaces the
   * localities correspond to define the backend that should register the tag.
   *
   * @param[in] myLocalityID The ID of my locality. Optional; the default assumes
   *                         a local tag and reads zero.
   * @param[in] remotes      An iterator over memory spaces that the tag may
   *                         interact with. Optional; by default, this is an empty
   *                         array (<tt>nullptr</tt>).
   * @param[in] remotes_end  An iterator in end position that matches \a remotes.
   *                         Optional; by default, this is equal to \a remotes
   *                         (i.e., signifying an empty set of memory spaces).
   * @return Returns a the newly created tag
   *
   * @see allocateMemorySlot for more details regarding \a remotes and locality
   *      IDs.
   *
   * \note A tag with an empty remote array may still be useful for separating
   *       local memory copies from remote ones. The resulting tag is a dubbed a
   *       local tag, whereas tags with more than one localities are global tags.
   *
   * If \a remotes is non-empty, then for all memory spaces \a remotes iterates
   * over, this call must be matched by a remote call to createTag (i.e., the
   * call must be collective across all participating memory spaces).
   *
   * The elements \a remotes iterates over must match across all memory spaces
   * that participate in the collective call, and must iterate over the remotes in
   * matching order.
   *
   * This function may fail for the following reasons:
   *  -# there is a duplicate memory space in the \a remotes array;
   *  -# a related backend is out of resources to create a new memory slot;
   *  -# the order of memory spaces differs across the collective call.
   */
  template <typename FwdIt = MemorySpace *>
  Tag createTag(
    const size_t myLocalityID = 0,
    FwdIt remotes = nullptr,
    const FwdIt remotes_end = nullptr)
  {
    Tag tag(_tagCounter);
    _tagCounter++;
    return tag;
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
