/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file Provides memory spaces and memory slots.
 * @author A. N. Yzelman
 * @date 8/8/2023 (factored out of datamover.hpp)
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
 * called), any associated resources are immediately freed. If the HiCR
 * #MemorySpace class was responsible for allocating the underlying memory, then
 * that memory shall be freed also.
 *
 * \note This means that destroying a memory slot would immediately allow
 *       creating a new one, without running into resource constraints.
 */
class MemorySlot {

 private:

  typedef void * const ptr_t;

 public:

  /**
   * @returns The address of the memory region the slot has registered.
   *
   * This function when called on a valid MemorySlot instance may not fail.
   *
   * \todo This interface assumes any backend equates a memory slot to a
   */
  ptr_t& getPointer() const noexcept;

  /**
   * @returns The size of the memory region the slot has registered.
   *
   * This function when called on a valid MemorySlot instance may not fail.
   */
  size_t getSize() const noexcept;

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
  size_t nLocalities() const noexcept;

};

/**
 * Encapsulates a message tag.
 *
 * For asynchronous data movement, fences may operate on messages that share the
 * same tag; meaning, that while fencing on a single message or on a group of
 * messages that share a tag, other messages may remain in flight after the
 * fence completes.
 *
 * There are a limited set of tags exposed by the system.
 *
 * The TagSlot may be memcpy'd between HiCR instances that share the same
 * context. This implies that a TagSlot must be a plain-old-data type.
 *
 * \internal This also implies that a TagSlot must be some sort of shared union
 *           struct between backends.
 *
 * The size of the TagSlot shall always be a multiple of <tt>sizeof(int)</tt>.
 *
 * \internal This last requirement ensures we can have an array of tags and use
 *           memcpy to communicate just parts of it.
 */
class TagSlot {

 public:

	 // there used to be a constructor here, but that precludes any backend from
	 // managing a possibly constrained set of tags. Instead, we now use the same
	 // mechanism as for #MemorySlot to have it tie to specific backends--
	 // see MemorySpace::createTagSlot

 /**
  * @returns a unique numerical identifier corresponding to this tag.
  *
  * The returned value is unique within the current HiCR instance. If a tag
  * is shared with other HiCR instances, there is a guarantee that each HiCR
  * instance returns the same ID.
  *
  * A call to this function on any valid instance of a #TagSlot shall never
  * fail.
  *
  * \todo Do we really need this? We already specified that the TagSlot may be
  *       memcpy'd, so the TagSlot itself is already a unique identifier.
  *       Defining the return type to have some limited byte size also severely
  *       limits backends, and perhaps overly so.
  */
 uint128_t getID() const noexcept;

  /**
   * @returns The number of localities this tagSlot has been created with.
   *
   * This function never returns <tt>0</tt>. When given a valid TagSlot
   * instance, a call to this function never fails.
   *
   * When referring to localities corresponding to this tag, only IDs strictly
   * lower than the returned value are valid.
   */
  size_t nLocalities() const noexcept;

};

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
  * A memory space cannot be constructed -- it may only be retrieved from a
  * #Resource instance.
  */
 MemorySpace();

public:

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
 * @param[in] remotes An array of memory spaces that the memory slot may
 *                    interact with. Optional; by default, this is an empty array
 *                    (<tt>nullptr</tt>)
 *
 * When \a remotes are empty, a \em local memory slot will be returned. A local
 * memory slot can only be used within this MemorySpace, and can only be used to
 * refer to local source regions or local destination regions.
 *
 * A memory slot that is not local is called a \em global memory slot instead.
 * A global memory slot has \$f k \f$ \em localities, where \f$ k \f$ is the
 * length of the \a remotes array. Unlike a local memory slot, a global memory
 * slot \em can be used to refer to \em remote source regions or remote
 * destination regions; i.e., global memory slot can be used to request data
 * movement between different memory spaces.
 *
 * If there are no direct paths of communication between any pair in the set of
 * current memory space and the memory spaces in \a remotes, an exception will
 * be thrown.
 *
 * \warning Use of a returned memory slot within a memory copy that has as
 *          source or destination a memory slot that is not owned by this memory
 *          space, while that memory space was not given as part of
 *          \a interactsWith, invites undefined behaviour.
 *
 * This function may fail for the following reasons:
 *  -# out of memory;
 *  -# a related backend is out of resources to create a new memory slot;
 *  -# there is a duplicate memory space in the \a remotes array;
 *  -# if there is no direct path of communication possible between any pair in
 *     the set of the current memory space and all memory spaces in \a remotes.
 *
 * \todo Should we take iterators rather than request a raw array?
 */
 MemorySlot allocateMemorySlot(const size_t size, MemorySpace * const remotes = nullptr);

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
 * @param[in] remotes An array of memory spaces that the memory slot may
 *                    interact with. Optional; by default, this is an empty array
 *                    (<tt>nullptr</tt>)
 *
 * @see allocateMemorySlot for more details regarding \a remotes. Note that in
 *      particular, an empty array \a remotes on a successful call results in a
 *      local memory slot being returned, while a non-empty \a remotes results
 *      in a global memory slot being returned.
 *
 * Destroying a returned memory slot will \em not free the memory at \a addr.
 *
 * This function may fail for the following reasons:
 *  -# a related backend is out of resources to create a new memory slot;
 *  -# there is a duplicate memory space in the \a remotes array;
 *  -# if there is no direct path of communication possible between any pair in
 *     the set of the current memory space and all memory spaces in \a remotes.
 *
 * \todo Should we take iterators rather than request a raw array?
 */
 MemorySlot createMemorySlot(void* addr, const size_t size, MemorySpace * const remotes = nullptr);

 /**
  * Creates a tag slot for use with memory slots that are created via calls to
  * #allocateMemorySlot or #createMemorySlot within this same memory space.
  *
 * @param[in] remotes An array of memory spaces that the tag may interact with.
 *                    Optional; by default, this is an empty array,
 *                    <tt>nullptr</tt>.
 *
 * @see allocateMemorySlot for more details regarding \a interactsWith.
 *
 * This function may fail for one reason:
 *  -# there is a duplicate memory space in the \a remotes array;
 *  -# a related backend is out of resources to create a new memory slot.
 *
 * \todo Should we take iterators rather than request a raw array?
 */
 TagSlot createTagSlot(MemorySpace * const remotes = nullptr);

/*
 * This operation will resolve the release of the allocated memory space which this slot holds.
 * \todo NOTE (AJ) disabled since I didn't get why this should not be embedded in the slot destructor
 void MemoryResource::freeMemorySlot();
 */
};

} // end namespace HiCR

