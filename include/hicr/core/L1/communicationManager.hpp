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

#include <map>
#include <memory>
#include <vector>
#include <hicr/core/L0/localMemorySlot.hpp>
#include <hicr/core/L0/globalMemorySlot.hpp>
#include <hicr/core/L0/memorySpace.hpp>
#include <hicr/core/definitions.hpp>
#include <hicr/core/exceptions.hpp>

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
  typedef std::pair<L0::GlobalMemorySlot::globalKey_t, std::shared_ptr<L0::LocalMemorySlot>> globalKeyMemorySlotPair_t;

  /**
   * Type definition for an array that stores sets of memory slots, separated by global key
   */
  typedef std::map<L0::GlobalMemorySlot::globalKey_t, std::shared_ptr<L0::GlobalMemorySlot>> globalKeyToMemorySlotMap_t;

  /**
   * Type definition for a tag-mapped set of key-mapped memory slot arrays
   */
  typedef std::map<L0::GlobalMemorySlot::tag_t, globalKeyToMemorySlotMap_t> globalMemorySlotTagKeyMap_t;

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
  __INLINE__ void exchangeGlobalMemorySlots(const L0::GlobalMemorySlot::tag_t tag, const std::vector<globalKeyMemorySlotPair_t> &memorySlots)
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
  __INLINE__ std::shared_ptr<L0::GlobalMemorySlot> getGlobalMemorySlot(const L0::GlobalMemorySlot::tag_t tag, const L0::GlobalMemorySlot::globalKey_t globalKey)
  {
    auto globalSlotImpl = getGlobalMemorySlotImpl(tag, globalKey);

    if (globalSlotImpl != nullptr) { return globalSlotImpl; }
    else
    {
      // Locking access to prevent concurrency issues
      this->lock();

      // If the requested tag and key are not found, return empty storage
      if (_globalMemorySlotTagKeyMap.contains(tag) == false)
      {
        this->unlock();
        HICR_THROW_LOGIC("Requesting a global memory slot for a tag (%lu) that has not been registered.", tag);
      }
      if (_globalMemorySlotTagKeyMap.at(tag).contains(globalKey) == false)
      {
        this->unlock();
        for (auto elem : _globalMemorySlotTagKeyMap.at(tag)) { printf("For Tag %lu: Key %lu\n", tag, elem.first); }
        printf("But tag %lu does not contain globalKey = %lu\n", tag, globalKey);
        HICR_THROW_LOGIC("Requesting a global memory slot for a  global key (%lu) not registered within the tag (%lu).", globalKey, tag);
      }

      // Getting requested memory slot
      auto value = _globalMemorySlotTagKeyMap.at(tag).at(globalKey);

      // Releasing lock
      this->unlock();

      // Returning value
      return value;
    }
  }

  /**
   * De-registers a previously registered global memory slot
   *
   * \param[in] memorySlot Memory slot to deregister.
   */
  __INLINE__ void deregisterGlobalMemorySlot(std::shared_ptr<L0::GlobalMemorySlot> memorySlot)
  {
    // Getting memory slot global information
    const auto memorySlotTag       = memorySlot->getGlobalTag();
    const auto memorySlotGlobalKey = memorySlot->getGlobalKey();

    // Locking access to prevent concurrency issues
    this->lock();

    // Checking whether the memory slot is correctly registered as global
    if (_globalMemorySlotTagKeyMap.contains(memorySlotTag) == false)
    {
      this->unlock();
      HICR_THROW_LOGIC("Attempting to de-register a global memory slot but its tag/key pair is not registered in this backend");
    }

    // Removing memory slot from the global memory slot map
    _globalMemorySlotTagKeyMap.at(memorySlotTag).erase(memorySlotGlobalKey);

    // Releasing lock
    this->unlock();

    // Calling internal deregistration implementation
    deregisterGlobalMemorySlotImpl(memorySlot);
  }

  /**
   * Queries the backend to update the internal state of the memory slot.
   * One main use case of this function is to update the number of messages received and sent to/from this slot.
   * This is a non-blocking, non-collective function.
   *
   * \param[in] memorySlot Identifier of the memory slot to query for updates.
   */
  __INLINE__ void queryMemorySlotUpdates(std::shared_ptr<L0::LocalMemorySlot> memorySlot)
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
  __INLINE__ void memcpy(std::shared_ptr<L0::LocalMemorySlot> destination,
                         const size_t                         dst_offset,
                         std::shared_ptr<L0::LocalMemorySlot> source,
                         const size_t                         src_offset,
                         const size_t                         size)
  {
    // Getting slot sizes. This operation is thread-safe
    const auto srcSize = source->getSize();
    const auto dstSize = destination->getSize();

    // Making sure the memory slots exist and is not null
    const auto actualSrcSize = size + src_offset;
    const auto actualDstSize = size + dst_offset;

    // Checking size doesn't exceed slot size
    if (actualSrcSize > srcSize)
      HICR_THROW_RUNTIME("Memcpy size (%lu) + offset (%lu) = (%lu) exceeds source slot (%p) capacity (%lu).", size, src_offset, actualSrcSize, source->getPointer(), srcSize);

    // Checking size doesn't exceed slot size
    if (actualDstSize > dstSize)
      HICR_THROW_RUNTIME(
        "Memcpy size (%lu) + offset (%lu) = (%lu) exceeds destination slot (%p) capacity (%lu).", size, dst_offset, actualDstSize, destination->getPointer(), dstSize);

    // To enable concurrent memcpy operations, the implementation is executed outside the mutex zone
    // This means that the developer needs to make sure that the implementation is concurrency-safe,
    // and try not to access any of the internal Backend class fields without proper mutex locking

    // Now calling internal memcpy function to give us a function that satisfies the operation
    memcpyImpl(destination, dst_offset, source, src_offset, size);
  }

  /**
   * Instructs the backend to perform an asynchronous memory copy from
   * within a local memory slot, to within a global memory slot.
   *
   * @param[in] destination  The destination global memory slot
   * @param[in] dst_offset   The offset (in bytes) within \a destination at
   *                         \a dst_locality
   * @param[in] source       The local source memory slot
   * @param[in] src_offset   The offset (in bytes) within \a source at
   *                         \a src_locality
   * @param[in] size         The number of bytes to copy from the source to the
   *                         destination
   */
  __INLINE__ void memcpy(std::shared_ptr<L0::GlobalMemorySlot> destination,
                         const size_t                          dst_offset,
                         std::shared_ptr<L0::LocalMemorySlot>  source,
                         const size_t                          src_offset,
                         const size_t                          size)
  {
    // Getting slot sizes. This operation is thread-safe
    const auto srcSize = source->getSize();

    // Making sure the memory slots exist and is not null
    const auto actualSrcSize = size + src_offset;

    // Checking size doesn't exceed slot size
    if (actualSrcSize > srcSize)
      HICR_THROW_RUNTIME("Memcpy size (%lu) + offset (%lu) = (%lu) exceeds source slot (%p) capacity (%lu).", size, src_offset, actualSrcSize, source->getPointer(), srcSize);

    // Now calling internal memcpy function to give us a function that satisfies the operation
    memcpyImpl(destination, dst_offset, source, src_offset, size);
  }

  /**
   * Instructs the backend to perform an asynchronous memory copy from
   * within a global memory slot, to within a local memory slot.
   *
   * @param[in] destination  The destination local memory slot
   * @param[in] dst_offset   The offset (in bytes) within \a destination at
   *                         \a dst_locality
   * @param[in] source       The global source memory slot
   * @param[in] src_offset   The offset (in bytes) within \a source at
   *                         \a src_locality
   * @param[in] size         The number of bytes to copy from the source to the
   *                         destination
   */
  __INLINE__ void memcpy(std::shared_ptr<L0::LocalMemorySlot>  destination,
                         const size_t                          dst_offset,
                         std::shared_ptr<L0::GlobalMemorySlot> source,
                         const size_t                          src_offset,
                         const size_t                          size)
  {
    // Getting slot sizes. This operation is thread-safe
    const auto dstSize = destination->getSize();

    // Making sure the memory slots exist and is not null
    const auto actualDstSize = size + dst_offset;

    // Checking size doesn't exceed slot size
    if (actualDstSize > dstSize)
      HICR_THROW_RUNTIME(
        "Memcpy size (%lu) + offset (%lu) = (%lu) exceeds destination slot (%p) capacity (%lu).", size, dst_offset, actualDstSize, destination->getPointer(), dstSize);

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
  __INLINE__ void fence(const L0::GlobalMemorySlot::tag_t tag)
  {
    // To enable concurrent fence operations, the implementation is executed outside the mutex zone
    // This means that the developer needs to make sure that the implementation is concurrency-safe,
    // and try not to access any of the internal Backend class fields without proper mutex locking

    // Now call the proper fence, as implemented by the backend
    fenceImpl(tag);
  }

  /**
   * Fences locally on a local memory slot until a number of messages are sent/received
   *
   * This is a non-collective and blocking call; returning from this function
   * indicates that specific to this memory slot, incoming memory movement has completed \em and that
   * outgoing memory movement has left the local interface.
   *
   * \param[in] slot A local memory slot
   * \param[in] expectedSent number of messages to finish sending on slot before returning
   * \param[in] expectedRecvd number of messages to finish receiving on slot before returning
   */
  __INLINE__ void fence(std::shared_ptr<L0::LocalMemorySlot> slot, size_t expectedSent, size_t expectedRecvd)
  {
    // To enable concurrent fence operations, the implementation is executed outside the mutex zone
    // This means that the developer needs to make sure that the implementation is concurrency-safe,
    // and try not to access any of the internal Backend class fields without proper mutex locking

    // Now call the proper fence, as implemented by the backend
    fenceImpl(slot, expectedSent, expectedRecvd);
  }

  /**
   * Fences locally on a global, but locally allocated memory slot, until a number of messages are sent/received
   *
   * This is a non-collective and blocking call; returning from this function
   * indicates that specific to this locally allocated memory slot, incoming memory movement has completed \em and that
   * outgoing memory movement has left the local interface.
   *
   * \param[in] slot A global memory slot which is locally allocated
   * \param[in] expectedSent number of messages to finish sending on slot before returning
   * \param[in] expectedRecvd number of messages to finish receiving on slot before returning
   */
  __INLINE__ void fence(std::shared_ptr<L0::GlobalMemorySlot> slot, size_t expectedSent, size_t expectedRecvd)
  {
    // To enable concurrent fence operations, the implementation is executed outside the mutex zone
    // This means that the developer needs to make sure that the implementation is concurrency-safe,
    // and try not to access any of the internal Backend class fields without proper mutex locking

    // Now call the proper fence, as implemented by the backend
    fenceImpl(slot, expectedSent, expectedRecvd);
  }

  /**
   * This function ensures that the global memory slot is reserved exclusively for access by the caller.
   *
   * This function might (or might not) block the caller to satisfy the exclusion, if the lock is already held by another caller.
   *
   * @param[in] memorySlot The memory slot to reserve
   * @return true, if the lock was acquired successfully; false, otherwise
   */
  __INLINE__ bool acquireGlobalLock(std::shared_ptr<L0::GlobalMemorySlot> memorySlot)
  {
    // Getting memory slot global information
    const auto memorySlotTag       = memorySlot->getGlobalTag();
    const auto memorySlotGlobalKey = memorySlot->getGlobalKey();

    // Checking whether the memory slot is correctly registered as global
    if (_globalMemorySlotTagKeyMap.contains(memorySlotTag) == false)
      HICR_THROW_LOGIC("Attempting to lock a global memory slot but its tag/key pair is not registered in this backend");

    // Checking whether the memory slot is correctly registered as global
    if (_globalMemorySlotTagKeyMap.at(memorySlotTag).contains(memorySlotGlobalKey) == false)
      HICR_THROW_LOGIC("Attempting to lock a global memory slot but its tag/key pair is not registered in this backend");

    // Calling internal implementation
    return acquireGlobalLockImpl(memorySlot);
  }

  /**
   * This function releases a previously acquired lock on a global memory slot
   *
   * @param[in] memorySlot The memory slot to release
   */
  __INLINE__ void releaseGlobalLock(std::shared_ptr<L0::GlobalMemorySlot> memorySlot)
  {
    // Getting memory slot global information
    const auto memorySlotTag       = memorySlot->getGlobalTag();
    const auto memorySlotGlobalKey = memorySlot->getGlobalKey();

    // Locking access to prevent concurrency issues
    this->lock();

    // Checking whether the memory slot is correctly registered as global
    if (_globalMemorySlotTagKeyMap.contains(memorySlotTag) == false)
    {
      this->unlock();
      HICR_THROW_LOGIC("Attempting to release a global memory slot but its tag/key pair is not registered in this backend");
    }

    // Checking whether the memory slot is correctly registered as global
    if (_globalMemorySlotTagKeyMap.at(memorySlotTag).contains(memorySlotGlobalKey) == false)
    {
      this->unlock();
      HICR_THROW_LOGIC("Attempting to release a global memory slot but its tag/key pair is not registered in this backend");
    }

    // Releasing lock
    this->unlock();

    // Calling internal implementation
    releaseGlobalLockImpl(memorySlot);
  }

  /**
   * This function flushes pending send operations
   */
  __INLINE__ virtual void flushSent() {}

  /**
   * This function flushes receives registered at remote queue of receiver
   */
  __INLINE__ virtual void flushReceived() {}

  protected:

  /**
   * Registers a global memory slot from a given address.
   *
   * \param[in] memorySlot Newly created global memory slot to register
   *
   * \internal This function is only meant to be called internally
   */
  __INLINE__ void registerGlobalMemorySlot(std::shared_ptr<L0::GlobalMemorySlot> memorySlot)
  {
    // Getting memory slot information
    const auto tag       = memorySlot->getGlobalTag();
    const auto globalKey = memorySlot->getGlobalKey();

    // Locking access to prevent concurrency issues
    this->lock();

    // Adding memory slot to the global map (based on tag and key)
    _globalMemorySlotTagKeyMap[tag][globalKey] = memorySlot;

    // Releasing lock
    this->unlock();
  }

  /**
   * Backend-internal implementation of the deregisterGlobalMemorySlotImpl function
   *
   * \param[in] memorySlot Memory slot to deregister.
   */
  virtual void deregisterGlobalMemorySlotImpl(std::shared_ptr<HiCR::L0::GlobalMemorySlot> memorySlot) = 0;

  /**
   * Backend-internal implementation of the getGlobalMemorySlot function
   *
   * Retrieves the map of globally registered slots
   *
   * \param[in] tag Tag that identifies a subset of all global memory slots
   * \param[in] globalKey The sorting key inside the tag subset that distinguished between registered slots
   * \return The map of registered global memory slots, filtered by tag and mapped by key
   */
  virtual std::shared_ptr<L0::GlobalMemorySlot> getGlobalMemorySlotImpl(const L0::GlobalMemorySlot::tag_t tag, const L0::GlobalMemorySlot::globalKey_t globalKey) = 0;

  /**
   * Exchanges memory slots among different local instances of HiCR to enable global (remote) communication
   *
   * \param[in] tag Identifies a particular subset of global memory slots
   * \param[in] memorySlots Array of local memory slots to make globally accessible
   */
  virtual void exchangeGlobalMemorySlotsImpl(const HiCR::L0::GlobalMemorySlot::tag_t tag, const std::vector<globalKeyMemorySlotPair_t> &memorySlots) = 0;

  /**
   * Backend-internal implementation of the queryMemorySlotUpdates function
   *
   * \param[in] memorySlot Memory slot to query updates for.
   */
  virtual void queryMemorySlotUpdatesImpl(std::shared_ptr<HiCR::L0::LocalMemorySlot> memorySlot) = 0;

  /**
   * Backend-internal implementation of the locking of a mutual exclusion mechanism. By default, no concurrency is assumed
   */
  virtual void lock(){};

  /**
   * Backend-internal implementation of the unlocking of a mutual exclusion mechanism
   */
  virtual void unlock(){};

  /**
   * Backend-internal implementation of the memcpy function
   *
   * @param[in] destination  The destination local memory slot
   * @param[in] dst_offset   The offset (in bytes) within \a destination at \a dst_locality
   * @param[in] source       The source local memory slot
   * @param[in] src_offset   The offset (in bytes) within \a source at \a src_locality
   * @param[in] size         The number of bytes to copy from the source to the destination
   */
  virtual void memcpyImpl(std::shared_ptr<HiCR::L0::LocalMemorySlot> destination,
                          const size_t                               dst_offset,
                          std::shared_ptr<HiCR::L0::LocalMemorySlot> source,
                          const size_t                               src_offset,
                          const size_t                               size)
  {
    HICR_THROW_LOGIC("Local->Local memcpy operations are unsupported by the given backend");
  };

  /**
   * Backend-internal implementation of the memcpy function
   *
   * @param[in] destination  The destination global memory slot
   * @param[in] dst_offset   The offset (in bytes) within \a destination at \a dst_locality
   * @param[in] source       The source local memory slot
   * @param[in] src_offset   The offset (in bytes) within \a source at \a src_locality
   * @param[in] size         The number of bytes to copy from the source to the destination
   */
  virtual void memcpyImpl(std::shared_ptr<HiCR::L0::GlobalMemorySlot> destination,
                          const size_t                                dst_offset,
                          std::shared_ptr<HiCR::L0::LocalMemorySlot>  source,
                          const size_t                                src_offset,
                          const size_t                                size)
  {
    HICR_THROW_LOGIC("Local->Global memcpy operations are unsupported by the given backend");
  };

  /**
   * Backend-internal implementation of the memcpy function
   *
   * @param[in] destination  The destination local memory slot
   * @param[in] dst_offset   The offset (in bytes) within \a destination at \a dst_locality
   * @param[in] source       The source global memory slot
   * @param[in] src_offset   The offset (in bytes) within \a source at \a src_locality
   * @param[in] size         The number of bytes to copy from the source to the destination
   */
  virtual void memcpyImpl(std::shared_ptr<HiCR::L0::LocalMemorySlot>  destination,
                          const size_t                                dst_offset,
                          std::shared_ptr<HiCR::L0::GlobalMemorySlot> source,
                          const size_t                                src_offset,
                          const size_t                                size)
  {
    HICR_THROW_LOGIC("Global->Local memcpy operations are unsupported by the given backend");
  };

  /**
   * Backend-internal implementation of the fence on a local memory slot
   *
   * \param[in] slot A local memory slot
   * \param[in] expectedSent number of messages to finish sending on slot before returning
   * \param[in] expectedRcvd number of messages to finish receiving on slot before returning
   */
  virtual void fenceImpl(std::shared_ptr<L0::LocalMemorySlot> slot, size_t expectedSent, size_t expectedRcvd) {}
  /**
   * Backend-internal implementation of the fence on a global memory slot
   *
   * \param[in] slot A global (but locally allocated) memory slot
   * \param[in] expectedSent number of messages to finish sending on slot before returning
   * \param[in] expectedRcvd number of messages to finish receiving on slot before returning
   */
  virtual void fenceImpl(std::shared_ptr<L0::GlobalMemorySlot> slot, size_t expectedSent, size_t expectedRcvd) {}
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
  virtual bool acquireGlobalLockImpl(std::shared_ptr<HiCR::L0::GlobalMemorySlot> memorySlot) = 0;

  /**
   * Backend-specific implementation of the releaseGlobalLock function
   * @param[in] memorySlot See the releaseGlobalLock function
   */
  virtual void releaseGlobalLockImpl(std::shared_ptr<HiCR::L0::GlobalMemorySlot> memorySlot) = 0;

  /**
   * Storage for global tag/key associated global memory slot exchange
   */
  globalMemorySlotTagKeyMap_t _globalMemorySlotTagKeyMap;
};

} // namespace L1

} // namespace HiCR
