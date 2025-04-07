/*
 *   Copyright 2025 Huawei Technologies Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
#include <mutex>
#include <utility>
#include <vector>
#include <hicr/core/localMemorySlot.hpp>
#include <hicr/core/globalMemorySlot.hpp>
#include <hicr/core/memorySpace.hpp>
#include <hicr/core/definitions.hpp>
#include <hicr/core/exceptions.hpp>

namespace HiCR
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
  using globalKeyMemorySlotPair_t = std::pair<GlobalMemorySlot::globalKey_t, std::shared_ptr<LocalMemorySlot>>;

  /**
   * Type definition for an array that stores sets of memory slots, separated by global key
   */
  using globalKeyToMemorySlotMap_t = std::map<GlobalMemorySlot::globalKey_t, std::shared_ptr<GlobalMemorySlot>>;

  /**
   * Type definition for a tag-mapped set of key-mapped memory slot arrays
   */
  using globalMemorySlotTagKeyMap_t = std::map<GlobalMemorySlot::tag_t, globalKeyToMemorySlotMap_t>;

  /**
   * Default destructor
   */
  virtual ~CommunicationManager() = default;

  /**
   * Backend-internal implementation of the locking of a mutual exclusion mechanism.
   * By default, a single mutex can protect access to internal fields.
   * It is up to the application developer to ensure that the mutex is used correctly
   * and efficiently, e.g., grouping multiple operations under a single lock.
   *
   * \todo: Implement a more fine-grained locking mechanism, e.g., per map mutex or parallel maps,
   *        and expose thread-safe operations to the user.
   */
  virtual void lock() { _mutex.lock(); };

  /**
   * Backend-internal implementation of the unlocking of a mutual exclusion mechanism.
   * Same considerations as for lock() apply.
   */
  virtual void unlock() { _mutex.unlock(); };

  /**
   * Exchanges memory slots among different local instances of HiCR to enable global (remote) communication
   *
   * \param[in] tag Identifies a particular subset of global memory slots
   * \param[in] memorySlots Array of local memory slots to make globally accessible
   */
  __INLINE__ void exchangeGlobalMemorySlots(GlobalMemorySlot::tag_t tag, const std::vector<globalKeyMemorySlotPair_t> &memorySlots)
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
   *
   * \note This method is not thread-safe. It is up to the application developer to ensure that
   */
  __INLINE__ std::shared_ptr<GlobalMemorySlot> getGlobalMemorySlot(GlobalMemorySlot::tag_t tag, GlobalMemorySlot::globalKey_t globalKey)
  {
    auto globalSlotImpl = getGlobalMemorySlotImpl(tag, globalKey);

    if (globalSlotImpl != nullptr) { return globalSlotImpl; }
    else
    {
      // If the requested tag and key are not found, return empty storage
      if (_globalMemorySlotTagKeyMap.contains(tag) == false) { HICR_THROW_LOGIC("Requesting a global memory slot for a tag (%lu) that has not been registered.", tag); }
      if (_globalMemorySlotTagKeyMap.at(tag).contains(globalKey) == false)
      {
        for (const auto &elem : _globalMemorySlotTagKeyMap.at(tag)) { printf("For Tag %lu: Key %lu\n", tag, elem.first); }
        printf("But tag %lu does not contain globalKey = %lu\n", tag, globalKey);
        HICR_THROW_LOGIC("Requesting a global memory slot for a  global key (%lu) not registered within the tag (%lu).", globalKey, tag);
      }

      // Getting requested memory slot
      auto value = _globalMemorySlotTagKeyMap.at(tag).at(globalKey);

      // Returning value
      return value;
    }
  }

  /**
   * Serializes a global memory slot, belonging to the current instance, into a buffer that can be sent over the network
   * to other instances, without the need to engage in a collective operation.
   *
   * \param[in] globalSlot The global memory slot to serialize
   * \return A pointer to the serialized representation of the global memory slot in a newly allocated buffer.
   *         An exception is thrown if the backend does not support the operation
   * \note The user is responsible for freeing the buffer, using delete[]
   */
  virtual uint8_t *serializeGlobalMemorySlot(const std::shared_ptr<HiCR::GlobalMemorySlot> &globalSlot) const
  {
    HICR_THROW_LOGIC("Trying to serialize a global memory slot; this is not supported in this backend\n");
    return nullptr;
  }

  /**
   * Deserializes a global memory slot from a given buffer.
   *
   * @param[in] buffer The buffer to deserialize the global memory slot from
   * @param[in] tag The tag to associate with the deserialized global memory slot
   * @return A pointer to the deserialized global memory slot
   */
  virtual std::shared_ptr<GlobalMemorySlot> deserializeGlobalMemorySlot(uint8_t *buffer, GlobalMemorySlot::tag_t tag)
  {
    HICR_THROW_LOGIC("Trying to deserialize a global memory slot; this is not supported in this backend\n");
    return nullptr;
  }

  /**
   * Promotes a local memory slot to a global memory slot without collective exchange across all instances.
   * This is an alternative to the exchangeGlobalMemorySlots function, which is a collective operation, and
   * remains the primary way to promote local memory slots to global memory slots, since using a slot produced
   * by this promote operation requires communicating it via a pre-established communication channel.
   *
   * @param[in] localMemorySlot The local memory slot to promote
   * @param[in] tag The tag to associate with the promoted global memory slot
   * @return A pointer to the promoted global memory slot
   */
  virtual std::shared_ptr<GlobalMemorySlot> promoteLocalMemorySlot(const std::shared_ptr<LocalMemorySlot> &localMemorySlot, GlobalMemorySlot::tag_t tag)
  {
    HICR_THROW_LOGIC("This backend does not support one-sided promotion of local memory slots to global");
    return nullptr;
  }

  /**
   * De-registers a previously registered global memory slot. This can be a local operation,
   * and is a CommunicationManager internal operation. The slot is not destroyed, it can be used,
   * but can no longer be accessed via #getGlobalMemorySlot.
   *
   * \param[in] memorySlot Memory slot to deregister.
   *
   * \note This method is not thread-safe. It is up to the application developer to ensure that it is called with proper locking.
   */
  __INLINE__ void deregisterGlobalMemorySlot(const std::shared_ptr<GlobalMemorySlot> &memorySlot)
  {
    // Getting memory slot global information
    const auto memorySlotTag       = memorySlot->getGlobalTag();
    const auto memorySlotGlobalKey = memorySlot->getGlobalKey();

    // Checking whether the memory slot is correctly registered as global
    if (_globalMemorySlotTagKeyMap.contains(memorySlotTag) == false)
    {
      HICR_THROW_LOGIC("Attempting to de-register a global memory slot but its tag/key pair is not registered in this backend");
    }

    // Removing memory slot from the global memory slot map
    _globalMemorySlotTagKeyMap.at(memorySlotTag).erase(memorySlotGlobalKey);
    deregisterGlobalMemorySlotImpl(memorySlot);
  }

  /**
   * Destroys a (collectively exchanged) global memory slot. This operation is non-blocking and non-collective.
   * Its effects will be visible after the next call to #fence(GlobalMemorySlot::tag_t)
   * where the tag is the tag of the memory slot to destroy. That means that multiple slots
   * under the same tag can be destroyed in a single fence operation.
   *
   * If the memory slot has not been created through #exchangeGlobalMemorySlots, the behavior is undefined.
   *
   * \param[in] memorySlot Memory slot to destroy.
   *
   * \note This method is not thread-safe. It is up to the application developer to ensure that
   */
  __INLINE__ void destroyGlobalMemorySlot(const std::shared_ptr<GlobalMemorySlot> &memorySlot)
  {
    auto tag = memorySlot->getGlobalTag();
    // Implicit creation of the tag entry if it doesn't exist is desired here
    _globalMemorySlotsToDestroyPerTag[tag].push_back(memorySlot);
  }

  /**
   * Destroys a (locally promoted) global memory slot. This operation is blocking and non-collective.
   * The effects will be visible immediately after the call to this function.
   *
   * If the memory slot has not been created through #promoteLocalMemorySlot, the behavior is undefined.
   *
   * @param[in] memorySlot Memory slot to destroy.
   */
  virtual void destroyPromotedGlobalMemorySlot(const std::shared_ptr<GlobalMemorySlot> &memorySlot)
  {
    HICR_THROW_LOGIC("This backend does not support promoted global memory slots.");
  }

  /**
   * Queries the backend to update the internal state of the memory slot.
   * One main use case of this function is to update the number of messages received and sent to/from this slot.
   * This is a non-blocking, non-collective function.
   *
   * \param[in] memorySlot Identifier of the memory slot to query for updates.
   */
  __INLINE__ void queryMemorySlotUpdates(std::shared_ptr<LocalMemorySlot> memorySlot)
  {
    // Getting value by copy
    queryMemorySlotUpdatesImpl(std::move(memorySlot));
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
  __INLINE__ void memcpy(const std::shared_ptr<LocalMemorySlot> &destination,
                         size_t                                      dst_offset,
                         const std::shared_ptr<LocalMemorySlot> &source,
                         size_t                                      src_offset,
                         size_t                                      size)
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
  __INLINE__ void memcpy(const std::shared_ptr<GlobalMemorySlot> &destination,
                         size_t                                       dst_offset,
                         const std::shared_ptr<LocalMemorySlot>  &source,
                         size_t                                       src_offset,
                         size_t                                       size)
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
  __INLINE__ void memcpy(const std::shared_ptr<LocalMemorySlot>  &destination,
                         size_t                                       dst_offset,
                         const std::shared_ptr<GlobalMemorySlot> &source,
                         size_t                                       src_offset,
                         size_t                                       size)
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
   * as well as destructions, only for the specified tag.
   *
   * This call is thread-safe.
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
   */
  __INLINE__ void fence(GlobalMemorySlot::tag_t tag)
  {
    lock();
    // Now call the proper fence, as implemented by the backend
    fenceImpl(tag);

    // Clear the memory slots to destroy
    _globalMemorySlotsToDestroyPerTag[tag].clear();
    _globalMemorySlotsToDestroyPerTag.erase(tag);
    unlock();
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
  __INLINE__ void fence(const std::shared_ptr<LocalMemorySlot> &slot, size_t expectedSent, size_t expectedRecvd)
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
  __INLINE__ void fence(const std::shared_ptr<GlobalMemorySlot> &slot, size_t expectedSent, size_t expectedRecvd)
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
  __INLINE__ bool acquireGlobalLock(const std::shared_ptr<GlobalMemorySlot> &memorySlot)
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
  __INLINE__ void releaseGlobalLock(const std::shared_ptr<GlobalMemorySlot> &memorySlot)
  {
    // Getting memory slot global information
    const auto memorySlotTag       = memorySlot->getGlobalTag();
    const auto memorySlotGlobalKey = memorySlot->getGlobalKey();

    // Checking whether the memory slot is correctly registered as global
    if (_globalMemorySlotTagKeyMap.contains(memorySlotTag) == false)
    {
      HICR_THROW_LOGIC("Attempting to release a global memory slot but its tag/key pair is not registered in this backend");
    }

    // Checking whether the memory slot is correctly registered as global
    if (_globalMemorySlotTagKeyMap.at(memorySlotTag).contains(memorySlotGlobalKey) == false)
    {
      HICR_THROW_LOGIC("Attempting to release a global memory slot but its tag/key pair is not registered in this backend");
    }

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
  __INLINE__ void registerGlobalMemorySlot(const std::shared_ptr<GlobalMemorySlot> &memorySlot)
  {
    // Getting memory slot information
    const auto tag       = memorySlot->getGlobalTag();
    const auto globalKey = memorySlot->getGlobalKey();

    // Adding memory slot to the global map (based on tag and key)
    _globalMemorySlotTagKeyMap[tag][globalKey] = memorySlot;
  }

  /**
   * Backend-internal implementation of the getGlobalMemorySlot function
   *
   * Retrieves the map of globally registered slots
   *
   * \param[in] tag Tag that identifies a subset of all global memory slots
   * \param[in] globalKey The sorting key inside the tag subset that distinguished between registered slots
   * \return The map of registered global memory slots, filtered by tag and mapped by key
   */
  virtual std::shared_ptr<GlobalMemorySlot> getGlobalMemorySlotImpl(GlobalMemorySlot::tag_t tag, GlobalMemorySlot::globalKey_t globalKey) = 0;

  /**
   * Exchanges memory slots among different local instances of HiCR to enable global (remote) communication
   *
   * \param[in] tag Identifies a particular subset of global memory slots
   * \param[in] memorySlots Array of local memory slots to make globally accessible
   */
  virtual void exchangeGlobalMemorySlotsImpl(GlobalMemorySlot::tag_t tag, const std::vector<globalKeyMemorySlotPair_t> &memorySlots) = 0;

  /**
   * Backend-internal implementation of the queryMemorySlotUpdates function
   *
   * \param[in] memorySlot Memory slot to query updates for.
   */
  virtual void queryMemorySlotUpdatesImpl(std::shared_ptr<LocalMemorySlot> memorySlot) = 0;

  /**
   * Optional backend-internal implementation of the deregisterGlobalMemorySlot function
   *
   * \param[in] memorySlot Memory slot to deregister.
   */
  virtual void deregisterGlobalMemorySlotImpl(const std::shared_ptr<GlobalMemorySlot> &memorySlot) {};

  /**
   * Deletes a global memory slot from the backend. This operation is collective.
   * Attempting to access the global memory slot after this operation will result in undefined behavior.
   *
   * \param[in] memorySlot Memory slot to destroy.
   */
  virtual void destroyGlobalMemorySlotImpl(std::shared_ptr<GlobalMemorySlot> memorySlot) = 0;

  /**
   * Backend-internal implementation of the memcpy function
   *
   * @param[in] destination  The destination local memory slot
   * @param[in] dst_offset   The offset (in bytes) within \a destination at \a dst_locality
   * @param[in] source       The source local memory slot
   * @param[in] src_offset   The offset (in bytes) within \a source at \a src_locality
   * @param[in] size         The number of bytes to copy from the source to the destination
   */
  virtual void memcpyImpl(const std::shared_ptr<LocalMemorySlot> &destination,
                          size_t                                      dst_offset,
                          const std::shared_ptr<LocalMemorySlot> &source,
                          size_t                                      src_offset,
                          size_t                                      size)
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
  virtual void memcpyImpl(const std::shared_ptr<GlobalMemorySlot> &destination,
                          size_t                                       dst_offset,
                          const std::shared_ptr<LocalMemorySlot>  &source,
                          size_t                                       src_offset,
                          size_t                                       size)
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
  virtual void memcpyImpl(const std::shared_ptr<LocalMemorySlot>  &destination,
                          size_t                                       dst_offset,
                          const std::shared_ptr<GlobalMemorySlot> &source,
                          size_t                                       src_offset,
                          size_t                                       size)
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
  virtual void fenceImpl(const std::shared_ptr<LocalMemorySlot> &slot, size_t expectedSent, size_t expectedRcvd) {}
  /**
   * Backend-internal implementation of the fence on a global memory slot
   *
   * \param[in] slot A global (but locally allocated) memory slot
   * \param[in] expectedSent number of messages to finish sending on slot before returning
   * \param[in] expectedRcvd number of messages to finish receiving on slot before returning
   */
  virtual void fenceImpl(const std::shared_ptr<GlobalMemorySlot> &slot, size_t expectedSent, size_t expectedRcvd) {}
  /**
   * Backend-internal implementation of the fence function
   *
   * \param[in] tag A tag that releases all processes that share the same value once they have arrived at it
   *
   */
  virtual void fenceImpl(GlobalMemorySlot::tag_t tag) = 0;

  /**
   * Backend-specific implementation of the acquireGlobalLock function
   * @param[in] memorySlot See the acquireGlobalLock function
   * @return See the acquireGlobalLock function
   */
  virtual bool acquireGlobalLockImpl(std::shared_ptr<GlobalMemorySlot> memorySlot) = 0;

  /**
   * Backend-specific implementation of the releaseGlobalLock function
   * @param[in] memorySlot See the releaseGlobalLock function
   */
  virtual void releaseGlobalLockImpl(std::shared_ptr<GlobalMemorySlot> memorySlot) = 0;

  /**
   * Accessor for the Global slots to destroy per tag field
   * @return A reference to the Global slots to destroy per tag field
   */
  [[nodiscard]] __INLINE__ auto &getGlobalMemorySlotsToDestroyPerTag() { return _globalMemorySlotsToDestroyPerTag; }

  /**
   * Accessor for the Global slots/tag key map
   * @return A reference to the Global slots/tag key map
   */
  [[nodiscard]] __INLINE__ auto &getGlobalMemorySlotTagKeyMap() { return _globalMemorySlotTagKeyMap; }

  protected:

  /**
   *  Increases the message receive counter for a given memory slot
   * 
   * @param[in] memorySlot The provided memory slot
   */
  __INLINE__ void increaseMessageRecvCounter(HiCR::LocalMemorySlot &memorySlot) noexcept { memorySlot.increaseMessagesRecv(); }

  /**
   *  Increases the message sent counter for a given memory slot
   * 
   * @param[in] memorySlot The provided memory slot
   */
  __INLINE__ void increaseMessageSentCounter(HiCR::LocalMemorySlot &memorySlot) noexcept { memorySlot.increaseMessagesSent(); }

  /**
   * Sets the message receive counter for a given memory slot
   * 
   * @param[in] memorySlot The provided memory slot
   * @param[in] count The new value to set for the counter
   */
  __INLINE__ void setMessagesRecv(HiCR::LocalMemorySlot &memorySlot, const size_t count) noexcept { memorySlot.setMessagesRecv(count); }

  /**
   * Sets the message sent counter for a given memory slot
   * 
   * @param[in] memorySlot The provided memory slot
   * @param[in] count The new value to set for the counter
   */
  __INLINE__ void setMessagesSent(HiCR::LocalMemorySlot &memorySlot, const size_t count) noexcept { memorySlot.setMessagesSent(count); }

  private:

  /**
   * Allow programmers to use a mutex to protect the communication manager's state when doing opeartions like
   * deregistering and marking memory slots for destruction, internally protect backend (e.g., MPI) calls, etc.
   */
  std::mutex _mutex;

  /**
   * Storage for global tag/key associated global memory slot exchange
   */
  globalMemorySlotTagKeyMap_t _globalMemorySlotTagKeyMap;

  /**
   * Storage for global memory slots to be destroyed during the next fence operation
   */
  std::map<GlobalMemorySlot::tag_t, std::vector<std::shared_ptr<GlobalMemorySlot>>> _globalMemorySlotsToDestroyPerTag;
};

} // namespace HiCR
