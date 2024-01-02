/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file communicationManager.hpp
 * @brief Provides a definition for the base backend's communication manager class.
 * @author S. M. Martin
 * @date 19/12/2023
 */

#pragma once

#include <hicr/L0/localMemorySlot.hpp>  
#include <hicr/L0/globalMemorySlot.hpp> 
#include <hicr/L0/memorySpace.hpp>
#include <hicr/common/definitions.hpp>
#include <hicr/common/exceptions.hpp>
#include <map>

namespace HiCR
{

namespace L1
{

/**
 * Encapsulates a HiCR Backend Communication Manager.
 *
 * Backends represent plugins to HiCR that provide support for a communication or device library. By adding new plugins developers extend HiCR's support for new hardware and software technologies.
 *
 * Backends need to fulfill the abstract virtual functions described here, so that HiCR can perform communication operations on the supported device/network library.
 *
 */
class CommunicationManager
{
  public:

  /**
   * Type definition for a global key / local memory slot pair
   */
  typedef std::pair<L0::GlobalMemorySlot::globalKey_t, L0::LocalMemorySlot *> globalKeyMemorySlotPair_t;

  /**
   * Common definition of a map that links key ids with global memory slot id arrays (for global exchange)
   */
  typedef parallelHashMap_t<L0::GlobalMemorySlot::globalKey_t, std::vector<L0::GlobalMemorySlot *>> memorySlotIdArrayMap_t;

  /**
   * Type definition for an array that stores sets of memory slots, separated by global key
   */
  typedef parallelHashMap_t<L0::GlobalMemorySlot::globalKey_t, L0::GlobalMemorySlot *> globalKeyToMemorySlotMap_t;

  /**
   * Type definition for a tag-mapped set of key-mapped memory slot arrays
   */
  typedef parallelHashMap_t<L0::GlobalMemorySlot::tag_t, globalKeyToMemorySlotMap_t> globalMemorySlotTagKeyMap_t;

  /**
   * Default destructor
   */
  virtual ~CommunicationManager() = default;

  /**
   * Exchanges memory slots among different local instances of HiCR to enable global (remote) communication
   *
   * \param[in] tag Identifies a particular subset of global memory slots
   * \param[in] memorySlots Array of local memory slots to make globally accessible
   */
  __USED__ inline void exchangeGlobalMemorySlots(const L0::GlobalMemorySlot::tag_t tag, const std::vector<globalKeyMemorySlotPair_t> &memorySlots)
  {
    // Calling internal implementation of this function
    exchangeGlobalMemorySlotsImpl(tag, memorySlots);
  }

  /**
   * Retrieves the map of globally registered slots
   *
   * \param[in] tag Tag that identifies a subset of all global memory slots
   * \param[in] globalKey The sorting key inside the tag subset that distinguished between registered slots
   * \return The map of registered global memory slots, filtered by tag and mapped by key
   */
  __USED__ inline L0::GlobalMemorySlot *getGlobalMemorySlot(const L0::GlobalMemorySlot::tag_t tag, const L0::GlobalMemorySlot::globalKey_t globalKey)
  {
    // If the requested tag and key are not found, return empty storage
    if (_globalMemorySlotTagKeyMap.contains(tag) == false) HICR_THROW_LOGIC("Requesting a global memory slot for a tag (%lu) that has not been registered.", tag);
    if (_globalMemorySlotTagKeyMap.at(tag).contains(globalKey) == false) HICR_THROW_LOGIC("Requesting a global memory slot for a  global key (%lu) not registered within the tag (%lu).", globalKey, tag);

    // Getting requested memory slot
    return _globalMemorySlotTagKeyMap.at(tag).at(globalKey);
  }

  /**
   * De-registers a previously registered global memory slot
   *
   * \param[in] memorySlot Memory slot to deregister.
   */
  __USED__ inline void deregisterGlobalMemorySlot(L0::GlobalMemorySlot *const memorySlot)
  {
    // Getting memory slot global information
    const auto memorySlotTag = memorySlot->getGlobalTag();
    const auto memorySlotGlobalKey = memorySlot->getGlobalKey();

    // Checking whether the memory slot is correctly registered as global
    if (_globalMemorySlotTagKeyMap.contains(memorySlotTag) == false) HICR_THROW_LOGIC("Attempting to de-register a global memory slot but its tag/key pair is not registered in this backend");

    // Calling internal implementation
    deregisterGlobalMemorySlotImpl(memorySlot);

    // Removing memory slot from the global memory slot map
    _globalMemorySlotTagKeyMap.at(memorySlotTag).erase(memorySlotGlobalKey);
  }

  /**
   * Queries the backend to update the internal state of the memory slot.
   * One main use case of this function is to update the number of messages received and sent to/from this slot.
   * This is a non-blocking, non-collective function.
   *
   * \param[in] memorySlot Identifier of the memory slot to query for updates.
   */
  __USED__ inline void queryMemorySlotUpdates(L0::GlobalMemorySlot *memorySlot)
  {
    // Getting value by copy
    queryMemorySlotUpdatesImpl(memorySlot);
  }

  /**
   * Instructs the backend to perform an asynchronous memory copy from
   * within a local memory slot, to within a local memory slot.
   *
   * @param[in] source       The local source memory slot
   * @param[in] src_offset   The offset (in bytes) within \a source at
   *                         \a src_locality
   * @param[in] destination  The destination local memory slot
   * @param[in] dst_offset   The offset (in bytes) within \a destination at
   *                         \a dst_locality
   * @param[in] size         The number of bytes to copy from the source to the
   *                         destination
   */
  __USED__ inline void memcpy(L0::LocalMemorySlot *destination, const size_t dst_offset, L0::LocalMemorySlot *source, const size_t src_offset, const size_t size)
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
   * Instructs the backend to perform an asynchronous memory copy from
   * within a  global memory slot, to within a local memory slot.
   *
   * @param[in] source       The local source memory slot
   * @param[in] src_offset   The offset (in bytes) within \a source at
   *                         \a src_locality
   * @param[in] destination  The destination global memory slot
   * @param[in] dst_offset   The offset (in bytes) within \a destination at
   *                         \a dst_locality
   * @param[in] size         The number of bytes to copy from the source to the
   *                         destination
   */
  __USED__ inline void memcpy(L0::GlobalMemorySlot *destination, const size_t dst_offset, L0::LocalMemorySlot *source, const size_t src_offset, const size_t size)
  {
    // Getting slot sizes. This operation is thread-safe
    const auto srcSize = source->getSize();

    // Making sure the memory slots exist and is not null
    const auto actualSrcSize = size + src_offset;

    // Checking size doesn't exceed slot size
    if (actualSrcSize > srcSize) HICR_THROW_RUNTIME("Memcpy size (%lu) + offset (%lu) = (%lu) exceeds source slot (%lu) capacity (%lu).", size, src_offset, actualSrcSize, source, srcSize);

    // Now calling internal memcpy function to give us a function that satisfies the operation
    memcpyImpl(destination, dst_offset, source, src_offset, size);
  }

  /**
   * Instructs the backend to perform an asynchronous memory copy from
   * within a  local memory slot, to within a global memory slot.
   *
   * @param[in] source       The global source memory slot
   * @param[in] src_offset   The offset (in bytes) within \a source at
   *                         \a src_locality
   * @param[in] destination  The destination local memory slot
   * @param[in] dst_offset   The offset (in bytes) within \a destination at
   *                         \a dst_locality
   * @param[in] size         The number of bytes to copy from the source to the
   *                         destination
   */
  __USED__ inline void memcpy(L0::LocalMemorySlot *destination, const size_t dst_offset, L0::GlobalMemorySlot *source, const size_t src_offset, const size_t size)
  {
    // Getting slot sizes. This operation is thread-safe
    const auto dstSize = destination->getSize();

    // Making sure the memory slots exist and is not null
    const auto actualDstSize = size + dst_offset;

    // Checking size doesn't exceed slot size
    if (actualDstSize > dstSize) HICR_THROW_RUNTIME("Memcpy size (%lu) + offset (%lu) = (%lu) exceeds destination slot (%lu) capacity (%lu).", size, dst_offset, actualDstSize, destination, dstSize);

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
  __USED__ inline void fence(const L0::GlobalMemorySlot::tag_t tag)
  {
    // To enable concurrent fence operations, the implementation is executed outside the mutex zone
    // This means that the developer needs to make sure that the implementation is concurrency-safe,
    // and try not to access any of the internal Backend class fields without proper mutex locking

    // Now call the proper fence, as implemented by the backend
    fenceImpl(tag);
  }

  /**
   * This function ensures that the global memory slot is reserved exclusively for access by the caller.
   *
   * This function might (or might not) block the caller to satisfy the exclusion, if the lock is already held by another caller.
   *
   * @param[in] memorySlot The memory slot to reserve
   * @return true, if the lock was acquired successfully; false, otherwise
   */
  __USED__ inline bool acquireGlobalLock(L0::GlobalMemorySlot *memorySlot)
  {
    // Getting memory slot global information
    const auto memorySlotTag = memorySlot->getGlobalTag();
    const auto memorySlotGlobalKey = memorySlot->getGlobalKey();

    // Checking whether the memory slot is correctly registered as global
    if (_globalMemorySlotTagKeyMap.contains(memorySlotTag) == false) HICR_THROW_LOGIC("Attempting to lock a global memory slot but its tag/key pair is not registered in this backend");

    // Checking whether the memory slot is correctly registered as global
    if (_globalMemorySlotTagKeyMap.at(memorySlotTag).contains(memorySlotGlobalKey) == false) HICR_THROW_LOGIC("Attempting to lock a global memory slot but its tag/key pair is not registered in this backend");

    // Calling internal implementation
    return acquireGlobalLockImpl(memorySlot);
  }

  /**
   * This function releases a previously acquired lock on a global memory slot
   *
   * @param[in] memorySlot The memory slot to release
   */
  __USED__ inline void releaseGlobalLock(L0::GlobalMemorySlot *memorySlot)
  {
    // Getting memory slot global information
    const auto memorySlotTag = memorySlot->getGlobalTag();
    const auto memorySlotGlobalKey = memorySlot->getGlobalKey();

    // Checking whether the memory slot is correctly registered as global
    if (_globalMemorySlotTagKeyMap.contains(memorySlotTag) == false) HICR_THROW_LOGIC("Attempting to release a global memory slot but its tag/key pair is not registered in this backend");

    // Checking whether the memory slot is correctly registered as global
    if (_globalMemorySlotTagKeyMap.at(memorySlotTag).contains(memorySlotGlobalKey) == false) HICR_THROW_LOGIC("Attempting to release a global memory slot but its tag/key pair is not registered in this backend");

    // Calling internal implementation
    releaseGlobalLockImpl(memorySlot);
  }

  /**
   * This function flushes pending memcpy operations
   */
  __USED__ virtual inline void flush() {}

  protected:

  /**
   * Registers a global memory slot from a given address.
   *
   * \param[in] memorySlot Newly created global memory slot to register
   *
   * \internal This function is only meant to be called internally
   */
  __USED__ inline void registerGlobalMemorySlot(L0::GlobalMemorySlot *memorySlot)
  {
    // Getting memory slot information
    const auto tag = memorySlot->getGlobalTag();
    const auto globalKey = memorySlot->getGlobalKey();

    // Sanity check: tag/globalkey collision
    if (_globalMemorySlotTagKeyMap.contains(tag) && _globalMemorySlotTagKeyMap.at(tag).contains(globalKey)) HICR_THROW_RUNTIME("Detected collision on global slots tag/globalKey (%lu/%lu). Another global slot was registered with that pair before.", tag, globalKey);

    // Adding memory slot to the global map (based on tag and key)
    _globalMemorySlotTagKeyMap[tag][globalKey] = memorySlot;
  }

  /**
   * Backend-internal implementation of the deregisterGlobalMemorySlotImpl function
   *
   * \param[in] memorySlot Memory slot to deregister.
   */
  virtual void deregisterGlobalMemorySlotImpl(L0::GlobalMemorySlot *memorySlot) = 0;

  /**
   * Exchanges memory slots among different local instances of HiCR to enable global (remote) communication
   *
   * \param[in] tag Identifies a particular subset of global memory slots
   * \param[in] memorySlots Array of local memory slots to make globally accessible
   */
  virtual void exchangeGlobalMemorySlotsImpl(const L0::GlobalMemorySlot::tag_t tag, const std::vector<globalKeyMemorySlotPair_t> &memorySlots) = 0;

  /**
   * Backend-internal implementation of the queryMemorySlotUpdates function
   *
   * \param[in] memorySlot Memory slot to query updates for.
   */
  virtual void queryMemorySlotUpdatesImpl(L0::GlobalMemorySlot *memorySlot) = 0;

  /**
   * Backend-internal implementation of the memcpy function
   *
   * @param[in] source       The source local memory slot
   * @param[in] src_offset   The offset (in bytes) within \a source at \a src_locality
   * @param[in] destination  The destination local memory slot
   * @param[in] dst_offset   The offset (in bytes) within \a destination at \a dst_locality
   * @param[in] size         The number of bytes to copy from the source to the destination
   */
  virtual void memcpyImpl(L0::LocalMemorySlot *destination, const size_t dst_offset, L0::LocalMemorySlot *source, const size_t src_offset, const size_t size) { HICR_THROW_LOGIC("Local->Local memcpy operations are unsupported by the given backend"); };

  /**
   * Backend-internal implementation of the memcpy function
   *
   * @param[in] source       The source local memory slot
   * @param[in] src_offset   The offset (in bytes) within \a source at \a src_locality
   * @param[in] destination  The destination global memory slot
   * @param[in] dst_offset   The offset (in bytes) within \a destination at \a dst_locality
   * @param[in] size         The number of bytes to copy from the source to the destination
   */
  virtual void memcpyImpl(L0::GlobalMemorySlot *destination, const size_t dst_offset, L0::LocalMemorySlot *source, const size_t src_offset, const size_t size) { HICR_THROW_LOGIC("Local->Global memcpy operations are unsupported by the given backend"); };

  /**
   * Backend-internal implementation of the memcpy function
   *
   * @param[in] source       The source global memory slot
   * @param[in] src_offset   The offset (in bytes) within \a source at \a src_locality
   * @param[in] destination  The destination local memory slot
   * @param[in] dst_offset   The offset (in bytes) within \a destination at \a dst_locality
   * @param[in] size         The number of bytes to copy from the source to the destination
   */
  virtual void memcpyImpl(L0::LocalMemorySlot *destination, const size_t dst_offset, L0::GlobalMemorySlot *source, const size_t src_offset, const size_t size) { HICR_THROW_LOGIC("Global->Local memcpy operations are unsupported by the given backend"); };

  /**
   * Backend-internal implementation of the fence function
   *
   * \param[in] tag A tag that releases all processes that share the same value once they have arrived at it
   *
   */
  virtual void fenceImpl(const L0::GlobalMemorySlot::tag_t tag) = 0;

  /**
   * Backend-specific implementation of the acquireGlobalLock function
   * @param[in] memorySlot See the acquireGlobalLock function
   * @return See the acquireGlobalLock function
   */
  virtual bool acquireGlobalLockImpl(L0::GlobalMemorySlot *memorySlot) = 0;

  /**
   * Backend-specific implementation of the releaseGlobalLock function
   * @param[in] memorySlot See the releaseGlobalLock function
   */
  virtual void releaseGlobalLockImpl(L0::GlobalMemorySlot *memorySlot) = 0;

  /**
   * Storage for global tag/key associated global memory slot exchange
   */
  globalMemorySlotTagKeyMap_t _globalMemorySlotTagKeyMap;
};

} // namespace L1

} // namespace HiCR
