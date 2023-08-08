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
 * Encapsulates a memory region that is either the source or destination of a
 * call to put/get operations.
 *
 * Each backend provides its own type of MemorySlot.
 *
 * There might be limited number of memory slots supported by a given backend.
 * When a memory slot goes out of scope (or otherwise has its destructor
 * called), any associated resources are immediately freed.
 *
 * \note This means that destroying a memory slot would immediately allow
 *       creating a new one, without running into resource constraints.
 */
class MemorySlot {

 private:

  typedef void * const ptr_t;

 public:

  /** @returns The address of the memory region the slot has registered. */
  ptr_t& getPointer() const noexcept;

  /** @returns The size of the memory region the slot has registered. */
  size_t getSize() const noexcept;

};

/**
 * This is a specialisation of the Resource class in HiCR, meant to express a
 * hardware memory element within a hierarchy (E.g., Cache, RAM, HBM, Device)
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
 */
class MemorySpace
{

 /**
 * Allocates a new memory slot within the memory resource.
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
 * @param[in] interactsWith An array of memory spaces that the memory slot may
 *                          interact with.
 *
 * \note This is required since some backends have limited memory slots, such as
 *       e.g. an Infiniband NIC which requires hardware buffers for each slot.
 *
 * \warning Use of a returned memory slot within a memory copy that has as
 *          source or destination a memory slot that is not owned by this memory
 *          space, while that memory space was not given as part of
 *          \a interactsWith, invites undefined behaviour.
 *
 * This function may fail for two reasons:
 *  -# out of memory;
 *  -# a related backend is out of resources to create a new memory slot.
 */
 MemorySlot MemoryResource::allocateMemorySlot(const size_t size, MemorySpace * const interactsWith = nullptr);

 /**
 * Creates a memory slot within the memory resource given an existing allocation
 *
 * @param[in] addr  The start address of the memory region to be registered.
 * @param[in] size  The size of the memory region to be registered.
 * @param[in] interactsWith An array of memory spaces that the memory slot may
 *                          interact with.
 *
 * @see allocateMemorySlot for more details regarding \a interactsWith.
 *
 * This function may fail for one reason:
 *  -# a related backend is out of resources to create a new memory slot.
 */
 MemorySlot MemoryResource::createMemorySlot(void* addr, const size_t size, MemorySpace * const interactsWith = nullptr);

/*
 * This operation will resolve the release of the allocated memory space which this slot holds.
 * \todo NOTE (AJ) disabled since I didn't get why this should not be embedded in the slot destructor
 void MemoryResource::freeMemorySlot();
 */
}

/**
 * A message tag.
 *
 * For asynchronous data movement, fences may operate on messages that share the
 * same tag; meaning, that while fencing on a single message or on a group of
 * messages that share a tag, other messages may remain in flight after the
 * fence completes.
 *
 * There are a limited set of tags exposed by the system.
 */
class TagSlot {
 public:
  /**
   * Constructs a new message and fence tag.
   *
   * There are a limited set of tags exposed by the system.
   *
   * @param[in] uuid A universally unique numerical tag
   */
  TagSlot(const uint128_t uuid);
}

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
 * Either \a source and \a src_locality, or \a destination and \a dst_locality,
 * \em must refer to a local memory region.
 *
 * A call to this function is one-sided, non-blocking, and, if the hardware and
 * network supports it, zero-copy.
 *
 * \note For blocking semantics, simply immediately follow this call to memcpy
 *       with a call to HiCR::fence().
 */
void Backend::memcpy(
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
 */
void fence(
 const TagSlot& tag,
 const size_t msgs_in, const size_t msgs_out,
 const set<MemoryResource*>& dests,
 const set<MemoryResource*>& sources
);

