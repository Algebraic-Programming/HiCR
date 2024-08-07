/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file communicationManager.hpp
 * @brief This file implements the pthread-based communication manager support for the host (CPU) memory backend
 * @author S. M. Martin
 * @date 14/8/2023
 */

#pragma once

#include <cstring>
#include "pthread.h"
#include <hicr/core/L1/communicationManager.hpp>
#include <hicr/core/L0/localMemorySlot.hpp>
#include "../../L0/globalMemorySlot.hpp"

namespace HiCR
{

namespace backend
{

namespace host
{

namespace pthreads
{

namespace L1
{

/**
 * Implementation of the SharedMemory/HWloc-based communication manager for the HiCR Shared Memory Backend.
 *
 * This backend uses pthread-based mutexes and barriers to prevent concurrent access violations
 */
class CommunicationManager final : public HiCR::L1::CommunicationManager
{
  public:

  /**
   * Constructor for the memory manager class for the shared memory backend
   *
   * \param[in] fenceCount Specifies how many times a fence has to be called for it to release callers
   */
  CommunicationManager(const size_t fenceCount = 1)
    : HiCR::L1::CommunicationManager()
  {
    // Initializing barrier for fence operation
    pthread_barrier_init(&_barrier, NULL, fenceCount);

    // Initializing mutex object
    pthread_mutex_init(&_mutex, NULL);
  }

  /**
   * The destructor deletes all created barrier/mutex locks
   */
  ~CommunicationManager()
  {
    // Freeing barrier memory
    pthread_barrier_destroy(&_barrier);

    // Freeing mutex memory
    pthread_mutex_destroy(&_mutex);
  }

  __INLINE__ std::shared_ptr<HiCR::L0::GlobalMemorySlot> getGlobalMemorySlotImpl(const HiCR::backend::host::L0::GlobalMemorySlot::tag_t       tag,
                                                                                 const HiCR::backend::host::L0::GlobalMemorySlot::globalKey_t globalKey)
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

  private:

  /**
   * Stores a barrier object to check on a barrier operation
   */
  pthread_barrier_t _barrier;

  /**
   * A mutex to make sure threads do not bother each other during certain operations
   */
  pthread_mutex_t _mutex;

  __INLINE__ void deregisterGlobalMemorySlotImpl(std::shared_ptr<HiCR::L0::GlobalMemorySlot> memorySlot) override
  {
    // Nothing to do here
  }

  __INLINE__ void exchangeGlobalMemorySlotsImpl(const HiCR::L0::GlobalMemorySlot::tag_t tag, const std::vector<globalKeyMemorySlotPair_t> &memorySlots) override
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
      auto globalMemorySlot = std::make_shared<HiCR::backend::host::L0::GlobalMemorySlot>(tag, globalKey, memorySlot);

      // Registering memory slot
      registerGlobalMemorySlot(globalMemorySlot);
      _shadowMap[tag][globalKey] = globalMemorySlot;
    }

    // Do not allow any thread to continue until the exchange is made
    barrier();
  }

  __INLINE__ void queryMemorySlotUpdatesImpl(std::shared_ptr<HiCR::L0::LocalMemorySlot> memorySlot) override
  {
    // This function should check and update the abstract class for completed memcpy operations
  }

  /**
   * A barrier implementation that synchronizes all threads in the HiCR instance
   */
  __INLINE__ void barrier() { pthread_barrier_wait(&_barrier); }

  /**
   * Implementation of the fence operation for the shared memory backend. In this case, nothing needs to be done, as
   * the system's memcpy operation is synchronous. This means that it's mere execution (whether immediate or deferred)
   * ensures its completion.
   */
  __INLINE__ void fenceImpl(const HiCR::L0::GlobalMemorySlot::tag_t tag) override { barrier(); }

  __INLINE__ void memcpyImpl(std::shared_ptr<HiCR::L0::LocalMemorySlot> destination,
                             const size_t                               dst_offset,
                             std::shared_ptr<HiCR::L0::LocalMemorySlot> source,
                             const size_t                               src_offset,
                             const size_t                               size) override
  {
    // Getting slot pointers
    const auto srcPtr = source->getPointer();
    const auto dstPtr = destination->getPointer();

    // Calculating actual offsets
    const auto actualSrcPtr = (void *)((uint8_t *)srcPtr + src_offset);
    const auto actualDstPtr = (void *)((uint8_t *)dstPtr + dst_offset);

    // Running memcpy now
    std::memcpy(actualDstPtr, actualSrcPtr, size);
  }

  __INLINE__ void memcpyImpl(std::shared_ptr<HiCR::L0::GlobalMemorySlot> destination,
                             const size_t                                dst_offset,
                             std::shared_ptr<HiCR::L0::LocalMemorySlot>  source,
                             const size_t                                src_offset,
                             const size_t                                size) override
  {
    // Getting up-casted pointer for the execution unit
    auto dst = dynamic_pointer_cast<HiCR::L0::GlobalMemorySlot>(destination);

    // Checking whether the execution unit passed is compatible with this backend
    if (dst == NULL) HICR_THROW_LOGIC("The passed destination memory slot is not supported by this backend\n");

    // Checking whether the memory slot is local. This backend only supports local data transfers
    if (dst->getSourceLocalMemorySlot() == nullptr) HICR_THROW_LOGIC("The passed destination memory slot is not local (required by this backend)\n");

    // Executing actual memcpy
    memcpy(dst->getSourceLocalMemorySlot(), dst_offset, source, src_offset, size);

    // Increasing message received/sent counters for both memory slots
    destination->getSourceLocalMemorySlot()->increaseMessagesRecv();
    source->increaseMessagesSent();
  }

  __INLINE__ void memcpyImpl(std::shared_ptr<HiCR::L0::LocalMemorySlot>  destination,
                             const size_t                                dst_offset,
                             std::shared_ptr<HiCR::L0::GlobalMemorySlot> source,
                             const size_t                                src_offset,
                             const size_t                                size) override
  {
    // Getting up-casted pointer for the execution unit
    auto src = dynamic_pointer_cast<HiCR::L0::GlobalMemorySlot>(source);

    // Checking whether the memory slot is compatible with this backend
    if (src == NULL) HICR_THROW_LOGIC("The passed source memory slot is not supported by this backend\n");

    // Checking whether the memory slot is local. This backend only supports local data transfers
    if (src->getSourceLocalMemorySlot() == nullptr) HICR_THROW_LOGIC("The passed source memory slot is not local (required by this backend)\n");

    // Executing actual memcpy
    memcpy(destination, dst_offset, src->getSourceLocalMemorySlot(), src_offset, size);

    // Increasing message received/sent counters for both memory slots
    destination->increaseMessagesRecv();
    source->getSourceLocalMemorySlot()->increaseMessagesSent();
  }

  __INLINE__ bool acquireGlobalLockImpl(std::shared_ptr<HiCR::L0::GlobalMemorySlot> memorySlot) override
  {
    // Getting up-casted pointer for the execution unit
    auto m = dynamic_pointer_cast<host::L0::GlobalMemorySlot>(memorySlot);

    // Checking whether the execution unit passed is compatible with this backend
    if (m == NULL) HICR_THROW_LOGIC("The passed memory slot is not supported by this backend\n");

    // Locking mutex
    return m->trylock();
  }

  __INLINE__ void releaseGlobalLockImpl(std::shared_ptr<HiCR::L0::GlobalMemorySlot> memorySlot) override
  {
    // Getting up-casted pointer for the execution unit
    auto m = dynamic_pointer_cast<host::L0::GlobalMemorySlot>(memorySlot);

    // Checking whether the execution unit passed is compatible with this backend
    if (m == NULL) HICR_THROW_LOGIC("The passed memory slot is not supported by this backend\n");

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

} // namespace L1

} // namespace pthreads

} // namespace host

} // namespace backend

} // namespace HiCR
