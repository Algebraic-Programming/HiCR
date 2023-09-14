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

#include <future>
#include <mutex>
#include <set>
#include <atomic>

#include <hicr/common/definitions.hpp>
#include <hicr/common/exceptions.hpp>
#include <hicr/processingUnit.hpp>

namespace HiCR
{

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

 /**
  * Type definition for a generic memory space identifier
  */
 typedef uint64_t memorySpaceId_t;

 /**
  * Type definition for a generic memory slot identifier
  */
 typedef uint64_t memorySlotId_t;

 /**
  * Common definition of a collection of compute resources
  */
 typedef std::set<computeResourceId_t> computeResourceList_t;

 /**
  * Common definition of a collection of memory spaces
  */
 typedef std::set<memorySlotId_t> memorySpaceList_t;

 /**
  * Type definition for a tag (identifies and groups operations and allows them to be waited upon)
  */
 typedef uint64_t tagId_t;

 virtual ~Backend() = default;

 /**
  * This function prompts the backend to perform the necessary steps to discover and list the compute resources provided by the library which it supports.
  *
  * In case of change in resource availability during runtime, users need to re-run this function to be able to see the changes.
  *
  * \internal It does not return anything because we want to allow users to run only once, and then consult it many times without having to make a copy.
  */
 __USED__ inline void queryComputeResources()
 {
   // Clearing existing compute resources
   _computeResourceList.clear();

   // Calling backend-internal implementation
   _computeResourceList = queryComputeResourcesImpl();
 }

 /**
  * This function prompts the backend to perform the necessary steps to discover and list the memory spaces provided by the library which it supports.
  *
  * In case of change in resource availability during runtime, users need to re-run this function to be able to see the changes.
  *
  * \internal It does not return anything because we want to allow users to run only once, and then consult it many times without having to make a copy.
  */
 __USED__ inline void queryMemorySpaces()
 {
   // Clearing existing memory space entries
   _memorySpaceList.clear();

   // Calling backend-internal implementation
   _memorySpaceList = queryMemorySpacesImpl();
 }

 /**
  * This function returns the list of queried compute resources, as visible by the backend.
  *
  * If this function is called before queryResources, then it will return an empty container.
  *
  * @return The list of compute resources, as detected the last time \a queryResources was executed.
  */
 __USED__ inline const computeResourceList_t &getComputeResourceList() { return _computeResourceList; }

 /**
  * This function returns the list of queried memory spaces, as visible by the backend.
  *
  * If this function is called before queryResources, then it will return an empty container.
  *
  * @return The list of memory spaces, as detected the last time \a queryResources was executed.
  */
 __USED__ inline const memorySpaceList_t &getMemorySpaceList() { return _memorySpaceList; }

 /**
  * This function returns the available allocatable size provided by the given memory space
  *
  * @param[in] memorySpace The memory space to query
  * @return The allocatable size within that memory space
  */
 __USED__ inline size_t getMemorySpaceSize(const memorySpaceId_t memorySpace) const
 {
   // Checking whether the referenced memory space actually exists
   if (_memorySpaceList.contains(memorySpace) == false) HICR_THROW_RUNTIME("Attempting to get size from memory space that does not exist (%lu) in this backend", memorySpace);

   // Calling internal implementation
   return getMemorySpaceSizeImpl(memorySpace);
 }

 /**
  * Creates a new processing unit from the provided compute resource
  *
  * \param[in] resource This is the identifier of the compute resource to use to instantiate into a processing unit. The identifier should be one of those provided by the backend. Providing an arbitrary identifier may lead to unexpected behavior.
  *
  * @return A unique pointer to the newly created processing unit. It is important to preserve the uniqueness of this object, since it represents a physical resource (e.g., core) and we do not want to assign it to multiple workers.
  */
 __USED__ inline std::unique_ptr<ProcessingUnit> createProcessingUnit(computeResourceId_t resource) const
 {
   // Checking whether the referenced compute resource actually exists
   if (_computeResourceList.contains(resource) == false) HICR_THROW_RUNTIME("Attempting to create processing unit from a compute resource that does not exist (%lu) in this backend", resource);

   // Calling internal implementation
   return createProcessingUnitImpl(resource);
 }

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
  * @param[in] launchType   Specifies whether the resolution of the operation hould be executed immediately or deferred until the fence
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
 __USED__ inline void memcpy(memorySlotId_t destination, const size_t dst_offset, const memorySlotId_t source, const size_t src_offset, const size_t size, const tagId_t &tag)
 {
   // Making sure the memory slots exist and is not null
   if (isMemorySlotValid(source) == false) HICR_THROW_RUNTIME("Invalid source memory slot(s) (%lu) provided. It either does not exist or is invalid", source);
   if (isMemorySlotValid(destination) == false) HICR_THROW_RUNTIME("Invalid destination memory slot(s) (%lu) provided. It either does not exist or is invalid", destination);

   // Getting slot sizes
   const auto srcSize = getMemorySlotSize(source);
   const auto dstSize = getMemorySlotSize(destination);

   // Making sure the memory slots exist and is not null
   const auto actualSrcSize = size + src_offset;
   const auto actualDstSize = size + dst_offset;
   if (actualSrcSize > srcSize) HICR_THROW_RUNTIME("Memcpy size (%lu) + offset (%lu) = (%lu) exceeds source slot (%lu) capacity (%lu).", size, src_offset, actualSrcSize, source, srcSize);
   if (actualDstSize > dstSize) HICR_THROW_RUNTIME("Memcpy size (%lu) + offset (%lu) = (%lu) exceeds destination slot (%lu) capacity (%lu).", size, dst_offset, actualDstSize, destination, dstSize);

   // Now calling internal memcpy function to give us a function that satisfies the operation
   memcpyImpl(destination, dst_offset, source, src_offset, size, tag);
 }

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
  *
  * \todo This all should be threading safe.
  */
 __USED__ inline void fence(const tagId_t tag)
 {
   // Now call the proper fence, as implemented by the backend
   fenceImpl(tag);
 }

 /**
  * Allocates memory in the specified memory space
  *
  * \param[in] memorySpaceId Memory space to allocate memory in
  * \param[in] size Size of the memory slot to create
  * \return A newly allocated memory slot in the specified memory space
  */
 __USED__ inline memorySlotId_t allocateMemorySlot(const memorySpaceId_t memorySpaceId, const size_t size)
 {
   // Checks whether the size requested exceeds the memory space size
   auto maxSize = getMemorySpaceSize(memorySpaceId);
   if (size > maxSize) HICR_THROW_LOGIC("Attempting to allocate more memory (%lu) than available in the memory space (%lu)", size, maxSize);

   // Increase + swap memory slot for thread-safety
   auto newMemorySlotId = _currentMemorySlotId++;

   // Calling internal implementation
   allocateMemorySlotImpl(memorySpaceId, size, newMemorySlotId);

   // Adding allocated memory slot to the set
   _allocatedMemorySlotSet.insert(newMemorySlotId);

   // Returning the id of the new memory slot
   return newMemorySlotId;
 }

 /**
  * Registers a memory slot from a given address
  *
  * \param[in] addr Pointer to the start of the memory slot
  * \param[in] size Size of the memory slot to create
  * \return A newly created memory slot
  */
 virtual memorySlotId_t registerMemorySlot(void *const addr, const size_t size)
 {
  // Increase + swap memory slot for thread-safety
  auto newMemorySlotId = _currentMemorySlotId++;

  // Calling internal implementation
  registerMemorySlotImpl(addr, size, newMemorySlotId);

  // Adding created memory slot to the set
  _registeredMemorySlotSet.insert(newMemorySlotId);

  // Returning the id of the new memory slot
  return newMemorySlotId;
 }

 /**
  * De-registers a memory slot previously registered
  *
  * \param[in] memorySlotId Identifier of the memory slot to deregister.
  */
 __USED__ inline void deregisterMemorySlot(memorySlotId_t memorySlotId)
 {
  // Checking whether the slot has been allocated with this backend
  if (_registeredMemorySlotSet.contains(memorySlotId) == false)
   HICR_THROW_LOGIC("Attempting to de-register a memory slot (%lu) that was not registered to this backend", memorySlotId);

  // Calling internal implementation
  deregisterMemorySlotImpl(memorySlotId);

  // Adding created memory slot to the set
  _registeredMemorySlotSet.erase(memorySlotId);
 }

 /**
  * Frees up a memory slot reserved from this memory space
  *
  * \param[in] memorySlotId Identifier of the memory slot to free up. It becomes unusable after freeing.
  */
 __USED__ inline void freeMemorySlot(memorySlotId_t memorySlotId)
 {
  // Checking whether the slot has been allocated with this backend
  if (_allocatedMemorySlotSet.contains(memorySlotId) == false)
   HICR_THROW_LOGIC("Attempting to free a memory slot (%lu) that was not allocated by this backend", memorySlotId);

  // Actually freeing up slot
  freeMemorySlotImpl(memorySlotId);

  // Removing entry from the set
  _allocatedMemorySlotSet.erase(memorySlotId);
 }

 /**
  * Obtains the local pointer from a given memory slot.
  *
  * \param[in] memorySlotId Identifier of the slot from where to source the pointer.
  * \return The local memory pointer, if applicable. NULL, otherwise.
  */
 __USED__ inline void *getMemorySlotLocalPointer(const memorySlotId_t memorySlotId) const
 {
  // Checking whether the slot has been associated with this backend
  if (_allocatedMemorySlotSet.contains(memorySlotId) == false && _registeredMemorySlotSet.contains(memorySlotId) == false)
   HICR_THROW_LOGIC("Attempting to get the size a memory slot (%lu) that is not associated to this backend", memorySlotId);

  // Calling backend-specific function
  return getMemorySlotLocalPointerImpl(memorySlotId);
 }

 /**
  * Obtains the size of the memory slot
  *
  * \param[in] memorySlotId Identifier of the slot from where to source the size.
  * \return The non-negative size of the memory slot, if applicable. Zero, otherwise.
  */
 __USED__ inline size_t getMemorySlotSize(const memorySlotId_t memorySlotId) const
 {
  // Checking whether the slot has been associated with this backend
  if (_allocatedMemorySlotSet.contains(memorySlotId) == false && _registeredMemorySlotSet.contains(memorySlotId) == false)
   HICR_THROW_LOGIC("Attempting to get the size a memory slot (%lu) that is not associated to this backend", memorySlotId);

  // Calling backend-specific function
  return getMemorySlotSizeImpl(memorySlotId);
 }

 /**
  * Checks whether the memory slot id exists and is a valid slot (e.g., the pointer is not NULL)
  *
  * \param[in] memorySlotId Identifier of the slot to check
  * \return True, if the referenced memory slot exists and is valid; false, otherwise
  */
 __USED__ inline bool isMemorySlotValid(const memorySlotId_t memorySlotId) const
 {
  // Checking whether the slot has been allocated with this backend
  if (_allocatedMemorySlotSet.contains(memorySlotId) == false && _registeredMemorySlotSet.contains(memorySlotId) == false) return false;

  // Running the implementation function
  return isMemorySlotValidImpl(memorySlotId);
 }

 protected:

 /**
  * Backend-internal implementation of the getMemorySlotLocalPointer function
  *
  * \param[in] memorySlotId Identifier of the slot from where to source the pointer.
  * \return The local memory pointer, if applicable. NULL, otherwise.
  */
 virtual void *getMemorySlotLocalPointerImpl(const memorySlotId_t memorySlotId) const = 0;

 /**
  * Backend-internal implementation of the getMemorySlotSize function
  *
  * \param[in] memorySlotId Identifier of the slot from where to source the size.
  * \return The non-negative size of the memory slot, if applicable. Zero, otherwise.
  */
 virtual size_t getMemorySlotSizeImpl(const memorySlotId_t memorySlotId) const = 0;

 /**
  * Backend-internal implementation of the isMemorySlotValid function
  *
  * \param[in] memorySlotId Identifier of the slot to check
  * \return True, if the referenced memory slot exists and is valid; false, otherwise
  */
 virtual bool isMemorySlotValidImpl(const memorySlotId_t memorySlotId) const = 0;

 /**
  * Backend-internal implementation of the getMemorySpaceSize function
  *
  * @param[in] memorySpace The memory space to query
  * @return The allocatable size within that memory space
  */
 virtual size_t getMemorySpaceSizeImpl(const memorySpaceId_t memorySpace) const = 0;

 /**
  * Backend-internal implementation of the createProcessingUnit function
  *
  * \param[in] resource This is the identifier of the compute resource to use to instantiate into a processing unit. The identifier should be one of those provided by the backend. Providing an arbitrary identifier may lead to unexpected behavior.
  *
  * @return A unique pointer to the newly created processing unit. It is important to preserve the uniqueness of this object, since it represents a physical resource (e.g., core) and we do not want to assign it to multiple workers.
  */
 virtual std::unique_ptr<ProcessingUnit> createProcessingUnitImpl(computeResourceId_t resource) const = 0;

 /**
  * Backend-internal implementation of the memcpy function
  *
  * @param[in] source       The source memory region
  * @param[in] src_offset   The offset (in bytes) within \a source at \a src_locality
  * @param[in] destination  The destination memory region
  * @param[in] dst_offset   The offset (in bytes) within \a destination at \a dst_locality
  * @param[in] size         The number of bytes to copy from the source to the destination
  * @param[in] tag          The tag of this memory copy
  */
 virtual void memcpyImpl(memorySlotId_t destination, const size_t dst_offset, const memorySlotId_t source, const size_t src_offset, const size_t size, const tagId_t &tag) = 0;

 /**
  * Backend-internal implementation of the fence function
  *
  * \param[in] tag Tag to execute a fence against
  */
 virtual void fenceImpl(const tagId_t tag) = 0;

 /**
  * Backend-internal implementation of the queryComputeResources function
  *
  * @return A list of compute resources
  */
 virtual computeResourceList_t queryComputeResourcesImpl() = 0;

 /**
  * Backend-internal implementation of the queryMemorySpaces function
  *
  * @return A list of memory spaces
  */
 virtual memorySpaceList_t queryMemorySpacesImpl() = 0;

 /**
  * Backend-internal implementation of the queryMemorySpaces function
  *
  *  \param[in] memorySpaceId Memory space to allocate memory in
  * \param[in] size Size of the memory slot to create
  * \param[in] The identifier for the new memory slot
  */
 virtual void allocateMemorySlotImpl(const memorySpaceId_t memorySpaceId, const size_t size, const memorySlotId_t memSlotId) = 0;

 /**
  * Backend-internal implementation of the registerMemorySlot function
  *
  * \param[in] addr Pointer to the start of the memory slot
  * \param[in] size Size of the memory slot to create
  * \param[in] The identifier for the new memory slot
  */
 virtual void registerMemorySlotImpl(void *const addr, const size_t size, const memorySlotId_t memSlotId) = 0;

 /**
   * Backend-internal implementation of the freeMemorySlot function
   *
   * \param[in] memorySlotId Identifier of the memory slot to free up. It becomes unusable after freeing.
   */
  virtual void freeMemorySlotImpl(memorySlotId_t memorySlotId) = 0;


  /**
   * Backend-internal implementation of the deregisterMemorySlot function
   *
   * \param[in] memorySlotId Identifier of the memory slot to deregister.
   */
  virtual void deregisterMemorySlotImpl(memorySlotId_t memorySlotId) = 0;

 private:

  /**
   * The internal container for the queried compute resources.
   */
  computeResourceList_t _computeResourceList;

  /**
   * The internal container for the queried memory spaces.
   */
  memorySpaceList_t _memorySpaceList;

  /**
   * Currently available slot id to be assigned. It should atomically increment as each slot is assigned
   */
  std::atomic<memorySlotId_t> _currentMemorySlotId = 0;

  /**
   * Stores the set of allocated (and unfreed) memory slots
   */
  parallelHashSet_t<memorySlotId_t> _allocatedMemorySlotSet;

  /**
   * Stores the set of registered memory slots
   */
  parallelHashSet_t<memorySlotId_t> _registeredMemorySlotSet;
};

} // namespace HiCR
