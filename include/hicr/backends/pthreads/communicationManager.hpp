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

/**
 * @file communicationManager.hpp
 * @brief This file implements the communication manager support for the Pthreads backend
 * @author S. M. Martin
 * @date 14/8/2023
 */

#pragma once

#include <cstring>
#include "pthread.h"
#include <hicr/core/communicationManager.hpp>
#include <hicr/core/localMemorySlot.hpp>
#include <hicr/backends/hwloc/globalMemorySlot.hpp>

namespace HiCR::backend::pthreads
{

/**
 * Implementation of the Pthreads communication manager
 *
 * This backend uses pthread-based mutexes and barriers to prevent concurrent access violations
 */
class CommunicationManager final : public HiCR::CommunicationManager
{
  public:

  /**
   * Constructor for the memory manager class for the Pthreads backend
   *
   * \param[in] fenceCount Specifies how many times a fence has to be called for it to release callers
   */
  CommunicationManager(const size_t fenceCount = 1)
    : HiCR::CommunicationManager()
  {
    // Initializing barrier for fence operation
    pthread_barrier_init(&_barrier, nullptr, fenceCount);

    // Initializing mutex object
    pthread_mutex_init(&_mutex, nullptr);
  }

  /**
   * The destructor deletes all created barrier/mutex locks
   */
  ~CommunicationManager() override
  {
    // Freeing barrier memory
    pthread_barrier_destroy(&_barrier);

    // Freeing mutex memory
    pthread_mutex_destroy(&_mutex);
  }

  __INLINE__ std::shared_ptr<HiCR::GlobalMemorySlot> getGlobalMemorySlotImpl(const HiCR::backend::hwloc::GlobalMemorySlot::tag_t       tag,
                                                                             const HiCR::backend::hwloc::GlobalMemorySlot::globalKey_t globalKey) override
  {
    if (_shadowMap.find(tag) != _shadowMap.end())
    {
      if (_shadowMap[tag].find(globalKey) != _shadowMap[tag].end())
        return _shadowMap[tag][globalKey];
      else
        return nullptr;
    }
    else { return nullptr; }
  }

  /**
   * Promotes a local memory slot to a global memory slot.
   * Not really needed for this backend, provided for PoC development
   *
   * \param[in] memorySlot Local memory slot to promote
   * \param[in] tag Tag to associate with the promoted global memory slot
   * \return The promoted global memory slot
   */
  __INLINE__ std::shared_ptr<HiCR::GlobalMemorySlot> promoteLocalMemorySlot(const std::shared_ptr<HiCR::LocalMemorySlot> &memorySlot, HiCR::GlobalMemorySlot::tag_t tag) override
  {
    // Creating new (generic) global memory slot
    auto globalMemorySlot = std::make_shared<HiCR::backend::hwloc::GlobalMemorySlot>(tag, 0 /* key */, memorySlot);

    // Returning the global memory slot
    return globalMemorySlot;
  }

  /**
   * Dummy override for the deregisterGlobalMemorySlot function, for PoC development
   *
   * \param[in] memorySlot Memory slot to deregister.
   */
  __INLINE__ void destroyPromotedGlobalMemorySlot(const std::shared_ptr<HiCR::GlobalMemorySlot> &memorySlot) override
  {
    // Nothing to do here
  }

  private:

  /**
   * Stores a barrier object to check on a barrier operation
   */
  pthread_barrier_t _barrier{};

  /**
   * A mutex to make sure threads do not bother each other during certain operations
   */
  pthread_mutex_t _mutex{};

  __INLINE__ void exchangeGlobalMemorySlotsImpl(const HiCR::GlobalMemorySlot::tag_t tag, const std::vector<globalKeyMemorySlotPair_t> &memorySlots) override
  {
    // Synchronize all intervening threads in this call
    barrier();

    // Simply adding local memory slots to the global map
    for (const auto &entry : memorySlots)
    {
      // Getting global key
      auto globalKey = entry.first;

      // Getting local memory slot to promot
      auto memorySlot = entry.second;

      // Creating new memory slot
      auto globalMemorySlot = std::make_shared<HiCR::backend::hwloc::GlobalMemorySlot>(tag, globalKey, memorySlot);

      // Registering memory slot
      registerGlobalMemorySlot(globalMemorySlot);
      _shadowMap[tag][globalKey] = globalMemorySlot;
    }

    // Do not allow any thread to continue until the exchange is made
    barrier();
  }

  __INLINE__ void queryMemorySlotUpdatesImpl(std::shared_ptr<HiCR::LocalMemorySlot> memorySlot) override
  {
    // This function should check and update the abstract class for completed memcpy operations
  }

  /**
   * A barrier implementation that synchronizes all threads in the HiCR instance
   */
  __INLINE__ void barrier() { pthread_barrier_wait(&_barrier); }

  /**
   * Implementation of the fence operation for the pthreads backend. In this case, nothing needs to be done, as
   * the system's memcpy operation is synchronous. This means that it's mere execution (whether immediate or deferred)
   * ensures its completion.
   */
  __INLINE__ void fenceImpl(const HiCR::GlobalMemorySlot::tag_t tag) override { barrier(); }

  __INLINE__ void memcpyImpl(const std::shared_ptr<HiCR::LocalMemorySlot> &destination,
                             const size_t                                  dst_offset,
                             const std::shared_ptr<HiCR::LocalMemorySlot> &source,
                             const size_t                                  src_offset,
                             const size_t                                  size) override
  {
    // Getting slot pointers
    const auto srcPtr = source->getPointer();
    const auto dstPtr = destination->getPointer();

    // Calculating actual offsets
    const auto actualSrcPtr = (void *)(static_cast<uint8_t *>(srcPtr) + src_offset);
    const auto actualDstPtr = (void *)(static_cast<uint8_t *>(dstPtr) + dst_offset);

    // Running memcpy now
    std::memcpy(actualDstPtr, actualSrcPtr, size);

    // Increasing recv/send counters
    increaseMessageRecvCounter(*destination);
    increaseMessageSentCounter(*source);
  }

  /**
   * Deletes a global memory slot from the backend.
   * Attempting to access the global memory slot after this operation will result in undefined behavior.
   *
   * \param[in] memorySlot Memory slot to destroy.
   */
  __INLINE__ void destroyGlobalMemorySlotImpl(std::shared_ptr<HiCR::GlobalMemorySlot> memorySlot) override
  {
    // Nothing to do here
  }

  __INLINE__ void memcpyImpl(const std::shared_ptr<HiCR::GlobalMemorySlot> &destination,
                             const size_t                                   dst_offset,
                             const std::shared_ptr<HiCR::LocalMemorySlot>  &source,
                             const size_t                                   src_offset,
                             const size_t                                   size) override
  {
    // Getting up-casted pointer for the execution unit
    auto dst = dynamic_pointer_cast<HiCR::GlobalMemorySlot>(destination);

    // Checking whether the execution unit passed is compatible with this backend
    if (dst == nullptr) HICR_THROW_LOGIC("The passed destination memory slot is not supported by this backend\n");

    // Checking whether the memory slot is local. This backend only supports local data transfers
    if (dst->getSourceLocalMemorySlot() == nullptr) HICR_THROW_LOGIC("The passed destination memory slot is not local (required by this backend)\n");

    // Executing actual memcpy
    memcpy(dst->getSourceLocalMemorySlot(), dst_offset, source, src_offset, size);

    // Increasing message received/sent counters for both memory slots
    increaseMessageRecvCounter(*destination->getSourceLocalMemorySlot());
    increaseMessageSentCounter(*source);
  }

  __INLINE__ void memcpyImpl(const std::shared_ptr<HiCR::LocalMemorySlot>  &destination,
                             const size_t                                   dst_offset,
                             const std::shared_ptr<HiCR::GlobalMemorySlot> &source,
                             const size_t                                   src_offset,
                             const size_t                                   size) override
  {
    // Getting up-casted pointer for the execution unit
    auto src = dynamic_pointer_cast<HiCR::GlobalMemorySlot>(source);

    // Checking whether the memory slot is compatible with this backend
    if (src == nullptr) HICR_THROW_LOGIC("The passed source memory slot is not supported by this backend\n");

    // Checking whether the memory slot is local. This backend only supports local data transfers
    if (src->getSourceLocalMemorySlot() == nullptr) HICR_THROW_LOGIC("The passed source memory slot is not local (required by this backend)\n");

    // Executing actual memcpy
    memcpy(destination, dst_offset, src->getSourceLocalMemorySlot(), src_offset, size);

    // Increasing message received/sent counters for both memory slots
    increaseMessageRecvCounter(*destination);
    increaseMessageSentCounter(*source->getSourceLocalMemorySlot());
  }

  __INLINE__ bool acquireGlobalLockImpl(std::shared_ptr<HiCR::GlobalMemorySlot> memorySlot) override
  {
    // Getting up-casted pointer for the execution unit
    auto m = dynamic_pointer_cast<hwloc::GlobalMemorySlot>(memorySlot);

    // Checking whether the execution unit passed is compatible with this backend
    if (m == nullptr) HICR_THROW_LOGIC("The passed memory slot is not supported by this backend\n");

    // Locking mutex
    return m->trylock();
  }

  __INLINE__ void releaseGlobalLockImpl(std::shared_ptr<HiCR::GlobalMemorySlot> memorySlot) override
  {
    // Getting up-casted pointer for the execution unit
    auto m = dynamic_pointer_cast<hwloc::GlobalMemorySlot>(memorySlot);

    // Checking whether the execution unit passed is compatible with this backend
    if (m == nullptr) HICR_THROW_LOGIC("The passed memory slot is not supported by this backend\n");

    // Locking mutex
    m->unlock();
  }

  __INLINE__ void lock() override
  {
    // Locking the pthread mutex
    pthread_mutex_lock(&_mutex);
  }

  __INLINE__ void unlock() override
  {
    // Locking the pthread mutex
    pthread_mutex_unlock(&_mutex);
  }

  private:

  // this map shadows the core HiCR map _globalMemorySlotTagKeyMap
  // to support getGlobalMemorySlot implementation
  // for this backend
  globalMemorySlotTagKeyMap_t _shadowMap;
};

} // namespace HiCR::backend::pthreads
