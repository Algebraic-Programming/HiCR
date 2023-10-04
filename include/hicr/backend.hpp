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

#include <atomic>
#include <future>
#include <mutex>
#include <set>

#include <hicr/common/definitions.hpp>
#include <hicr/common/exceptions.hpp>
#include <hicr/processingUnit.hpp>
#include <hicr/memorySlot.hpp>

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
   * Common definition of a collection of compute resources
   */
  typedef std::set<computeResourceId_t> computeResourceList_t;

  /**
   * Common definition of a collection of memory spaces
   */
  typedef std::set<memorySpaceId_t> memorySpaceList_t;

  /**
   * Common definition of a collection of memory slots
   */
  typedef std::map<memorySlotId_t, MemorySlot*> memorySlotMap_t;

  /**
   * Common definition of a map that links key ids with memory slot id arrays (for global exchange)
   */
  typedef std::map<globalKey_t, std::vector<MemorySlot*>> memorySlotIdArrayMap_t;

  /**
   * Type definition for a global key / memory slot pair
   */
  typedef std::pair<globalKey_t, MemorySlot*> globalKeyMemorySlotPair_t;

  /**
   * Type definition for pending local to global memory slot promotions
   */
  typedef std::map<tag_t, std::vector<globalKeyMemorySlotPair_t>> localToGlobalPromotionArray_t;

  /**
   * Type definition for an array that stores arrays of memory slots, separated by global key
   */
  typedef std::map<globalKey_t, std::vector<MemorySlot*>> globalKeyToMemorySlotArrayMap_t;

  /**
   * Type definition for a tag-mapped set of key-mapped memory slot arrays
   */
  typedef std::map<tag_t, globalKeyToMemorySlotArrayMap_t> globalMemorySlotTagKeyMap_t;

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
    // Lock Thread-safety mutex
    _mutex.lock();

    // Clearing existing compute resources
    _computeResourceList.clear();

    // Calling backend-internal implementation
    _computeResourceList = queryComputeResourcesImpl();

    // Release Thread-safety mutex
    _mutex.unlock();
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
    // Lock Thread-safety mutex
    _mutex.lock();

    // Clearing existing memory space entries
    _memorySpaceList.clear();

    // Calling backend-internal implementation
    _memorySpaceList = queryMemorySpacesImpl();

    // Release Thread-safety mutex
    _mutex.unlock();
  }

  /**
   * This function returns the list of queried compute resources, as visible by the backend.
   *
   * If this function is called before queryResources, then it will return an empty container.
   *
   * @return The list of compute resources, as detected the last time \a queryResources was executed.
   */
  __USED__ inline const computeResourceList_t getComputeResourceList()
  {
    // Lock Thread-safety mutex
    _mutex.lock();

    // Getting value by copy
    const auto value = _computeResourceList;

    // Release Thread-safety mutex
    _mutex.unlock();

    return value;
  }

  /**
   * This function returns the list of queried memory spaces, as visible by the backend.
   *
   * If this function is called before queryResources, then it will return an empty container.
   *
   * @return The list of memory spaces, as detected the last time \a queryResources was executed.
   */
  __USED__ inline const memorySpaceList_t getMemorySpaceList()
  {
    // Lock Thread-safety mutex
    _mutex.lock();

    // Getting value by copy
    const auto value = _memorySpaceList;

    // Release Thread-safety mutex
    _mutex.unlock();

    return value;
  }

  /**
   * This function returns the available allocatable size provided by the given memory space
   *
   * @param[in] memorySpace The memory space to query
   * @return The allocatable size within that memory space
   */
  __USED__ inline size_t getMemorySpaceSize(const memorySpaceId_t memorySpace)
  {
    // Lock Thread-safety mutex
    _mutex.lock();

    // Checking whether the referenced memory space actually exists
    if (_memorySpaceList.contains(memorySpace) == false) [[unlikely]]
    {
      // Release mutex before triggering the exception
      _mutex.unlock();

      // Triggering exception
      HICR_THROW_RUNTIME("Attempting to get size from memory space that does not exist (%lu) in this backend", memorySpace);
    }

    // Getting value by copy
    const auto value = getMemorySpaceSizeImpl(memorySpace);

    // Release Thread-safety mutex
    _mutex.unlock();

    // Calling internal implementation
    return value;
  }

  /**
   * Creates a new processing unit from the provided compute resource
   *
   * \param[in] resource This is the identifier of the compute resource to use to instantiate into a processing unit. The identifier should be one of those provided by the backend. Providing an arbitrary identifier may lead to unexpected behavior.
   *
   * @return A unique pointer to the newly created processing unit. It is important to preserve the uniqueness of this object, since it represents a physical resource (e.g., core) and we do not want to assign it to multiple workers.
   */
  __USED__ inline std::unique_ptr<ProcessingUnit> createProcessingUnit(computeResourceId_t resource)
  {
    // Lock Thread-safety mutex
    _mutex.lock();

    // Checking whether the referenced compute resource actually exists
    if (_computeResourceList.contains(resource) == false) [[unlikely]]
    {
      // Release mutex before triggering the exception
      _mutex.unlock();

      // Triggering exception
      HICR_THROW_RUNTIME("Attempting to create processing unit from a compute resource that does not exist (%lu) in this backend", resource);
    }

    // Getting value by copy
    auto value = createProcessingUnitImpl(resource);

    // Release Thread-safety mutex
    _mutex.unlock();

    // Calling internal implementation
    return value;
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
  __USED__ inline void memcpy(MemorySlot* destination, const size_t dst_offset, MemorySlot* source, const size_t src_offset, const size_t size)
  {
    // Making sure the memory slots exist and is not null. This operation is thread-safe
    if (isMemorySlotValid(source) == false) HICR_THROW_RUNTIME("Invalid source memory slot(s) (%lu) provided. It either does not exist or is invalid", source);
    if (isMemorySlotValid(destination) == false) HICR_THROW_RUNTIME("Invalid destination memory slot(s) (%lu) provided. It either does not exist or is invalid", destination);

    // Getting slot sizes. This operation is thread-safe
    const auto srcSize = source->getSize();
    const auto dstSize = destination->getSize();

    // Lock Thread-safety mutex
    _mutex.lock();

    // Making sure the memory slots exist and is not null
    const auto actualSrcSize = size + src_offset;
    const auto actualDstSize = size + dst_offset;

    // Checking size doesn't exceed slot size
    if (actualSrcSize > srcSize) [[unlikely]]
    {
      // Release mutex before triggering the exception
      _mutex.unlock();

      // Triggering exception
      HICR_THROW_RUNTIME("Memcpy size (%lu) + offset (%lu) = (%lu) exceeds source slot (%lu) capacity (%lu).", size, src_offset, actualSrcSize, source, srcSize);
    }

    // Checking size doesn't exceed slot size
    if (actualDstSize > dstSize) [[unlikely]]
    {
      // Release mutex before triggering the exception
      _mutex.unlock();

      // Triggering exception
      HICR_THROW_RUNTIME("Memcpy size (%lu) + offset (%lu) = (%lu) exceeds destination slot (%lu) capacity (%lu).", size, dst_offset, actualDstSize, destination, dstSize);
    }

    // Release Thread-safety mutex
    _mutex.unlock();

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
    // Lock Thread-safety mutex
    _mutex.lock();

    // Performing all pending local to global memory slot promotions now
    exchangeGlobalMemorySlots(tag, _pendingLocalToGlobalPromotions[tag]);

    // Clearing exchanged slot from the pending list
    _pendingLocalToGlobalPromotions.erase(tag);

    // Release Thread-safety mutex
    _mutex.unlock();

    // To enable concurrent fence operations, the implementation is executed outside the mutex zone
    // This means that the developer needs to make sure that the implementation is concurrency-safe,
    // and try not to access any of the internal Backend class fields without proper mutex locking

    // Now call the proper fence, as implemented by the backend
    fenceImpl(tag, _globalMemorySlotTagKeyMap[tag]);
  }

  /**
   * Allocates a local memory slotin the specified memory space
   *
   * \param[in] memorySpaceId Memory space to allocate memory in
   * \param[in] size Size of the memory slot to create
   * \return A newly allocated memory slot in the specified memory space
   */
  __USED__ inline MemorySlot* allocateLocalMemorySlot(const memorySpaceId_t memorySpaceId, const size_t size)
  {
    // Checks whether the size requested exceeds the memory space size. This is a thread-safe operation
    auto maxSize = getMemorySpaceSize(memorySpaceId);

    // Lock Thread-safety mutex
    _mutex.lock();

    // Checking size doesn't exceed slot size
    if (size > maxSize) [[unlikely]]
    {
      // Release mutex before triggering the exception
      _mutex.unlock();

      // Triggering exception
      HICR_THROW_LOGIC("Attempting to allocate more memory (%lu) than available in the memory space (%lu)", size, maxSize);
    }

    // Increase + swap memory slot for thread-safety
    auto newMemorySlotId = _currentMemorySlotId++;

    // Calling internal implementation. This is done inside the mutex zone because it is meant to be an
    // infrequent and fast operation, and ensuring concurrency safety is much more important than parallelism in this case
    auto ptr = allocateLocalMemorySlotImpl(memorySpaceId, size, newMemorySlotId);

    // Creating new memory slot structure
    auto newMemSlot = new MemorySlot(
      newMemorySlotId,
      ptr,
      size,
      MemorySlot::creationType_t::allocated,
      MemorySlot::localityType_t::local);

    // Adding allocated memory slot to the set
    _memorySlotMap.insert(std::make_pair(newMemorySlotId, newMemSlot));

    // Release Thread-safety mutex
    _mutex.unlock();

    // Returning the id of the new memory slot
    return newMemSlot;
  }

  /**
   * Registers a local memory slot from a given address
   *
   * \param[in] ptr Pointer to the start of the memory slot
   * \param[in] size Size of the memory slot to create
   * \return A newly created memory slot
   */
  virtual MemorySlot* registerLocalMemorySlot(void *const ptr, const size_t size)
  {
    // Lock Thread-safety mutex
    _mutex.lock();

    // Increase + swap memory slot for thread-safety
    auto newMemorySlotId = _currentMemorySlotId++;

    // Calling internal implementation. This is done inside the mutex zone because it is meant to be an
    // infrequent and fast operation, and ensuring concurrency safety is much more important than parallelism in this case
    registerLocalMemorySlotImpl(ptr, size, newMemorySlotId);

    // Creating new memory slot structure
    auto newMemSlot = new MemorySlot(
      newMemorySlotId,
      ptr,
      size,
      MemorySlot::creationType_t::registered,
      MemorySlot::localityType_t::local);

    // Adding created memory slot to the set
    _memorySlotMap.insert(std::make_pair(newMemorySlotId, newMemSlot));

    // Release Thread-safety mutex
    _mutex.unlock();

    // Returning the id of the new memory slot
    return newMemSlot;
  }

  /**
   * Promotes a local memory slot into a global memory slot
   *
   * \param[in] tag Groups a particular subset of global memory slots. Useful to separate global memory slot exchange for different independent purposes
   * \param[in] globalKey The key to sort the global slots within a given tag, so that the ordering is deterministic if all different keys are passed.
   * \param[in] localSlotId Local slot id to promote to global
   *
   * For this operation to take effect, it is required to run a fence operation afterwards.
   */
  virtual void promoteMemorySlotToGlobal(const tag_t tag, const globalKey_t globalKey, MemorySlot* memorySlot)
  {
    // Lock Thread-safety mutex
    _mutex.lock();

    // Getting memory slot Id
    const auto memorySlotId = memorySlot->getId();

    // Checking if the memory slot actually exists
    if (_memorySlotMap.contains(memorySlotId) == false) [[unlikely]]
    {
      // Release mutex before triggering the exception
      _mutex.unlock();

      // Triggering exception
      HICR_THROW_LOGIC("Attempting to promote to global a local a memory slot (%lu) that is not associated to this backend", memorySlotId);
    }

    // Calling internal implementation of this function
    _pendingLocalToGlobalPromotions[tag].push_back(std::make_pair(globalKey, memorySlot));

    // Release Thread-safety mutex
    _mutex.unlock();
  }

  /**
   * Retrieves the map of globally registered slots
   *
   * \return The map of registered global memory slots, filtered by tag and mapped by key
   */
  __USED__ inline globalMemorySlotTagKeyMap_t getGlobalMemorySlots()
  {
    // Lock Thread-safety mutex
    _mutex.lock();

    // Getting value by copy
    const auto value = _globalMemorySlotTagKeyMap;

    // Release Thread-safety mutex
    _mutex.unlock();

    return value;
  }

  /**
   * De-registers a previously registered memory slot
   *
   * \param[in] memorySlotId Identifier of the memory slot to deregister.
   */
  __USED__ inline void deregisterLocalMemorySlot(MemorySlot* const memorySlot)
  {
    // Lock Thread-safety mutex
    _mutex.lock();

    // Getting memory slot Id
    const auto memorySlotId = memorySlot->getId();

    // Checking whether the slot has been associated with this backend
    if (_memorySlotMap.contains(memorySlotId) == false) [[unlikely]]
    {
      // Release mutex before triggering the exception
      _mutex.unlock();

      // Triggering exception
      HICR_THROW_LOGIC("Attempting to de-register a memory slot (%lu) that is not associated to this backend", memorySlotId);
    }

    // Checking whether the slot is local
    if (_memorySlotMap.at(memorySlotId)->getLocalityType() != MemorySlot::localityType_t::local) [[unlikely]]
    {
      // Release mutex before triggering the exception
      _mutex.unlock();

      // Triggering exception
      HICR_THROW_LOGIC("Attempting to de-register a memory slot (%lu) that is not local to this backend", memorySlotId);
    }

    // Checking whether the slot has been registered
    if (_memorySlotMap.at(memorySlotId)->getCreationType() != MemorySlot::creationType_t::registered) [[unlikely]]
    {
      // Release mutex before triggering the exception
      _mutex.unlock();

      // Triggering exception
      HICR_THROW_LOGIC("Attempting to de-register a memory slot (%lu) that was not manually registered to this backend", memorySlotId);
    }

    // Calling internal implementation
    deregisterLocalMemorySlotImpl(memorySlot);

    // Removing memory slot from the set
    _memorySlotMap.erase(memorySlotId);

    // Release Thread-safety mutex
    _mutex.unlock();
  }

  /**
   * Frees up a memory slot reserved from this memory space
   *
   * \param[in] memorySlotId Identifier of the memory slot to free up. It becomes unusable after freeing.
   */
  __USED__ inline void freeLocalMemorySlot(MemorySlot* memorySlot)
  {
    // Lock Thread-safety mutex
    _mutex.lock();

    // Getting memory slot Id
    const auto memorySlotId = memorySlot->getId();

    // Checking whether the slot has been allocated with this backend
    if (_memorySlotMap.contains(memorySlotId) == false) [[unlikely]]
    {
      // Release mutex before triggering the exception
      _mutex.unlock();

      // Triggering exception
      HICR_THROW_LOGIC("Attempting to free a memory slot (%lu) that is not associated to this backend", memorySlotId);
    }

    // Checking whether the slot is local
    if (_memorySlotMap.at(memorySlotId)->getLocalityType() != MemorySlot::localityType_t::local) [[unlikely]]
    {
      // Release mutex before triggering the exception
      _mutex.unlock();

      // Triggering exception
      HICR_THROW_LOGIC("Attempting to free a memory slot (%lu) that is not local to this backend", memorySlotId);
    }

    // Checking whether the slot has been allocated with this backend
    if (_memorySlotMap.at(memorySlotId)->getCreationType() != MemorySlot::creationType_t::allocated) [[unlikely]]
    {
      // Release mutex before triggering the exception
      _mutex.unlock();

      // Triggering exception
      HICR_THROW_LOGIC("Attempting to free a memory slot (%lu) that was not allocated with this backend", memorySlotId);
    }

    // Actually freeing up slot
    freeLocalMemorySlotImpl(memorySlot);

    // Removing entry from the set
    _memorySlotMap.erase(memorySlotId);

    // Release Thread-safety mutex
    _mutex.unlock();
  }

  /**
   * Queries the backend to update the internal state of the memory slot.
   * One main use case of this function is to update the number of messages received and sent to/from this slot.
   * This is a non-blocking, non-collective function.
   *
   * \param[in] memorySlotId Identifier of the memory slot to query for updates.
   */
  __USED__ inline void queryMemorySlotUpdates(const MemorySlot* memorySlot)
  {
    // Lock Thread-safety mutex
    _mutex.lock();

    // Getting memory slot Id
    const auto memorySlotId = memorySlot->getId();

    // Checking whether the slot has been associated with this backend
    if (_memorySlotMap.contains(memorySlotId) == false) [[unlikely]]
    {
      // Release mutex before triggering the exception
      _mutex.unlock();

      // Triggering exception
      HICR_THROW_LOGIC("Attempting to query updates for a memory slot (%lu) that is not associated to this backend", memorySlotId);
    }

    // Getting value by copy
    queryMemorySlotUpdatesImpl(memorySlot);

    // Release Thread-safety mutex
    _mutex.unlock();
  }

  /**
   * Checks whether the memory slot id exists and is a valid slot (e.g., the pointer is not NULL)
   *
   * \param[in] memorySlotId Identifier of the slot to check
   * \return True, if the referenced memory slot exists and is valid; false, otherwise
   */
  __USED__ inline bool isMemorySlotValid(const MemorySlot* memorySlot)
  {
    // Lock Thread-safety mutex
    _mutex.lock();

    // Checking whether the slot has been associated with this backend
    if (_memorySlotMap.contains(memorySlot->getId()) == false) [[unlikely]]
    {
      // Release mutex before triggering the exception
      _mutex.unlock();

      // Triggering exception
      HICR_THROW_LOGIC("Attempting to get the size a memory slot (%lu) that is not associated to this backend", memorySlot->getId());
    }

    // Getting value by copy
    const auto value = isMemorySlotValidImpl(memorySlot);

    // Release Thread-safety mutex
    _mutex.unlock();

    // Running the implementation function
    return value;
  }

  protected:

  /**
   * Registers a global memory slot from a given address.
   *
   * \param[in] tag Represents the subgroup of HiCR instances that will share the global reference
   * \param[in] key Represents the a subset of memory slots that will be grouped together.
   *                 They will be sorted by this value, which allows for recognizing which slots came from which instance.
   * \param[in] ptr Pointer to the start of the memory slot
   * \param[in] size Size of the memory slot to create
   * \return A newly created memory slot
   *
   * \internal This function is only meant to be called internally and must be done within the a mutex zone.
   */
  virtual MemorySlot* registerGlobalMemorySlot(tag_t tag, globalKey_t key, void *const ptr, const size_t size)
  {
    // Increase + swap memory slot for thread-safety
    auto newMemorySlotId = _currentMemorySlotId++;

    // Creating new memory slot structure
    auto newMemorySlot = new MemorySlot(
      newMemorySlotId,
      ptr,
      size,
      MemorySlot::creationType_t::registered,
      MemorySlot::localityType_t::global,
      tag,
      key);

    // Adding created memory slot to the set
    _memorySlotMap.insert(std::make_pair(newMemorySlotId, newMemorySlot));

    // Adding memory slot to the global map (based on tag and key)
    _globalMemorySlotTagKeyMap[tag][key].push_back(newMemorySlot);

    // Returning the id of the new memory slot
    return newMemorySlot;
  }

  /**
   * Backend-internal implementation of the isMemorySlotValid function
   *
   * \param[in] memorySlotId Identifier of the slot to check
   * \return True, if the referenced memory slot exists and is valid; false, otherwise
   */
  virtual bool isMemorySlotValidImpl(const MemorySlot* memorySlotId) const = 0;

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
   */
  virtual void memcpyImpl(MemorySlot* destination, const size_t dst_offset, MemorySlot* source, const size_t src_offset, const size_t size) = 0;

  /**
   * Backend-internal implementation of the fence function
   *
   * \param[in] tag A tag that releases all processes that share the same value once they have arrived at it
   *
   */
  virtual void fenceImpl(const tag_t tag, const globalKeyToMemorySlotArrayMap_t& globalSlots) = 0;

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
   * Backend-internal implementation of the queryLocalMemorySlot function
   *
   * \param[in] memorySpaceId Memory space to allocate memory in
   * \param[in] size Size of the memory slot to create
   * \param[in] memSlotId The identifier for the new local memory slot
   * \return The internal pointer associated to the local memory slot
   */
  virtual void *allocateLocalMemorySlotImpl(const memorySpaceId_t memorySpaceId, const size_t size, const memorySlotId_t memSlotId) = 0;

  /**
   * Backend-internal implementation of the registerLocalMemorySlot function
   *
   * \param[in] addr Pointer to the start of the memory slot
   * \param[in] size Size of the memory slot to create
   * \param[in] memSlotId The identifier for the new local memory slot
   */
  virtual void registerLocalMemorySlotImpl(void *const addr, const size_t size, const memorySlotId_t memSlotId) = 0;

  /**
   * Backend-internal implementation of the freeLocalMemorySlot function
   *
   * \param[in] memorySlotId Identifier of the local memory slot to free up. It becomes unusable after freeing.
   */
  virtual void freeLocalMemorySlotImpl(MemorySlot* memorySlot) = 0;

  /**
   * Backend-internal implementation of the deregisterMemorySlot function
   *
   * \param[in] memorySlotId Identifier of the memory slot to deregister.
   */
  virtual void deregisterLocalMemorySlotImpl(MemorySlot* memorySlot) = 0;

  /**
   * Exchanges memory slots among different local instances of HiCR to enable global (remote) communication
   *
   * \param[in] tag Identifies a particular subset of global memory slots
   */
  virtual void exchangeGlobalMemorySlots(const tag_t tag, const std::vector<globalKeyMemorySlotPair_t>& memorySlots) = 0;

  /**
   * Backend-internal implementation of the queryMemorySlotUpdates function
   *
   * \param[in] memorySlotId Identifier of the memory slot to query for updates.
   */
  virtual void queryMemorySlotUpdatesImpl(const MemorySlot* memorySlot) = 0;

  private:

  /**
   * Mutual exclusion mechanism for thread safety
   */
  std::mutex _mutex;

  /**
   * Storage for global tag/key associated global memory slot exchange
   */
  globalMemorySlotTagKeyMap_t _globalMemorySlotTagKeyMap;

  /**
   * Array for temporarily holding pending local to global memory slot promotions
   */
  localToGlobalPromotionArray_t _pendingLocalToGlobalPromotions;

  /**
   * Stores the map of created memory slots
   */
  memorySlotMap_t _memorySlotMap;

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
};

} // namespace HiCR
