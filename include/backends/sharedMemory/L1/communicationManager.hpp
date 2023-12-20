/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file communicationManager.hpp
 * @brief This file implements the communication manager support for the shared memory backend
 * @author S. M. Martin
 * @date 14/8/2023
 */

#pragma once

#include "hwloc.h"
#include "pthread.h"
#include <backends/sharedMemory/L0/localMemorySlot.hpp>
#include <backends/sharedMemory/L0/globalMemorySlot.hpp>
#include <hicr/L1/communicationManager.hpp>

namespace HiCR
{

namespace backend
{

namespace sharedMemory
{

namespace L1
{

/**
 * Implementation of the SharedMemory/HWloc-based communication manager for the HiCR Shared Memory Backend.
 */
class CommunicationManager final : public HiCR::L1::CommunicationManager
{
  public:

  /**
   * Constructor for the memory manager class for the shared memory backend
   *
   * \param[in] fenceCount Specifies how many times a fence has to be called for it to release callers
   */
  CommunicationManager(const size_t fenceCount = 1) : HiCR::L1::CommunicationManager()
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

  private:

  /**
   * Stores a barrier object to check on a barrier operation
   */
  pthread_barrier_t _barrier;

  /**
   * A mutex to make sure threads do not bother each other during certain operations
   */
  pthread_mutex_t _mutex;

  /**
   * Specifies the biding support requested by the user. It should be by default strictly binding to follow HiCR's design, but can be relaxed upon request, when binding does not matter or a first touch policy is followed
   */
  L0::LocalMemorySlot::binding_type _hwlocBindingRequested = L0::LocalMemorySlot::binding_type::strict_binding;

  __USED__ inline void deregisterGlobalMemorySlotImpl(HiCR::L0::GlobalMemorySlot *memorySlot) override
  {
    // Nothing to do here
  }

  /**
   * Exchanges memory slots among different local instances of HiCR to enable global (remote) communication
   *
   * \param[in] tag Identifies a particular subset of global memory slots
   * \param[in] memorySlots Array of local memory slots to make globally accessible
   */
  __USED__ inline void exchangeGlobalMemorySlotsImpl(const HiCR::L0::GlobalMemorySlot::tag_t tag, const std::vector<globalKeyMemorySlotPair_t> &memorySlots) override
  {
    // Synchronize all intervening threads in this call
    barrier();

    // Doing the promotion of memory slots as a critical section
    pthread_mutex_lock(&_mutex);

    // Simply adding local memory slots to the global map
    for (const auto &entry : memorySlots)
    {
      // Getting global key
      auto globalKey = entry.first;

      // Getting local memory slot to promot
      auto memorySlot = entry.second;

      // Creating new memory slot
      auto globalMemorySlot = new L0::GlobalMemorySlot(tag, globalKey, memorySlot);

      // Registering memory slot
      registerGlobalMemorySlot(globalMemorySlot);
    }

    // Releasing mutex
    pthread_mutex_unlock(&_mutex);

    // Do not allow any thread to continue until the exchange is made
    barrier();
  }

  /**
   * Backend-internal implementation of the queryMemorySlotUpdates function
   *
   * \param[in] memorySlot Memory slot to query updates for.
   */
  __USED__ inline void queryMemorySlotUpdatesImpl(HiCR::L0::GlobalMemorySlot *memorySlot) override
  {
    // This function should check and update the abstract class for completed memcpy operations
  }

  /**
   * A barrier implementation that synchronizes all threads in the HiCR instance
   */
  __USED__ inline void barrier()
  {
    pthread_barrier_wait(&_barrier);
  }

  /**
   * Implementation of the fence operation for the shared memory backend. In this case, nothing needs to be done, as
   * the system's memcpy operation is synchronous. This means that it's mere execution (whether immediate or deferred)
   * ensures its completion.
   */
  __USED__ inline void fenceImpl(const HiCR::L0::GlobalMemorySlot::tag_t tag) override
  {
    barrier();
  }

  __USED__ inline void memcpyImpl(HiCR::L0::LocalMemorySlot *destination, const size_t dst_offset, HiCR::L0::LocalMemorySlot *source, const size_t src_offset, const size_t size) override
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

    __USED__ inline void memcpyImpl(HiCR::L0::GlobalMemorySlot *destination, const size_t dst_offset, HiCR::L0::LocalMemorySlot *source, const size_t src_offset, const size_t size) override
  {
    // Getting up-casted pointer for the execution unit
    auto dst = dynamic_cast<HiCR::L0::GlobalMemorySlot *>(destination);

    // Checking whether the execution unit passed is compatible with this backend
    if (dst == NULL) HICR_THROW_LOGIC("The passed destination memory slot is not supported by this backend\n");

    // Checking whether the memory slot is local. This backend only supports local data transfers
    if (dst->getSourceLocalMemorySlot() == nullptr) HICR_THROW_LOGIC("The passed destination memory slot is not local (required by this backend)\n");

    // Executing actual memcpy
    memcpy(dst->getSourceLocalMemorySlot(), dst_offset, source, src_offset, size);

    // Increasing message received/sent counters for memory slots
    dst->increaseMessagesRecv();
  }

   __USED__ inline void memcpyImpl(HiCR::L0::LocalMemorySlot *destination, const size_t dst_offset, HiCR::L0::GlobalMemorySlot *source, const size_t src_offset, const size_t size) override
  {
    // Getting up-casted pointer for the execution unit
    auto src = dynamic_cast<HiCR::L0::GlobalMemorySlot *>(source);

    // Checking whether the memory slot is compatible with this backend
    if (src == NULL) HICR_THROW_LOGIC("The passed source memory slot is not supported by this backend\n");

    // Checking whether the memory slot is local. This backend only supports local data transfers
    if (src->getSourceLocalMemorySlot() == nullptr) HICR_THROW_LOGIC("The passed source memory slot is not local (required by this backend)\n");
    
    // Executing actual memcpy
    memcpy(destination, dst_offset, src->getSourceLocalMemorySlot(), src_offset, size);

    // Increasing message received/sent counters for memory slots
    src->increaseMessagesSent();
  }

  __USED__ inline void memcpyImpl(HiCR::L0::GlobalMemorySlot *destination, const size_t dst_offset, HiCR::L0::GlobalMemorySlot *source, const size_t src_offset, const size_t size) override
  {
    // Getting up-casted pointer for the execution unit
    auto src = dynamic_cast<HiCR::L0::GlobalMemorySlot *>(source);

    // Checking whether the memory slot is compatible with this backend
    if (src == NULL) HICR_THROW_LOGIC("The passed source memory slot is not supported by this backend\n");

    // Checking whether the memory slot is local. This backend only supports local data transfers
    if (src->getSourceLocalMemorySlot() == nullptr) HICR_THROW_LOGIC("The passed source memory slot is not local (required by this backend)\n");

    // Getting up-casted pointer for the execution unit
    auto dst = dynamic_cast<HiCR::L0::GlobalMemorySlot *>(destination);

    // Checking whether the execution unit passed is compatible with this backend
    if (dst == NULL) HICR_THROW_LOGIC("The passed destination memory slot is not supported by this backend\n");

    // Checking whether the memory slot is local. This backend only supports local data transfers
    if (dst->getSourceLocalMemorySlot() == nullptr) HICR_THROW_LOGIC("The passed destination memory slot is not local (required by this backend)\n");
    
    // Executing actual memcpy
    memcpy(dst->getSourceLocalMemorySlot(), dst_offset, src->getSourceLocalMemorySlot(), src_offset, size);

    // Increasing message received/sent counters for memory slots
    src->increaseMessagesSent();
    dst->increaseMessagesRecv();
  }

  __USED__ inline bool acquireGlobalLockImpl(HiCR::L0::GlobalMemorySlot *memorySlot) override
  {
    // Getting up-casted pointer for the execution unit
    auto m = dynamic_cast<L0::GlobalMemorySlot *>(memorySlot);

    // Checking whether the execution unit passed is compatible with this backend
    if (m == NULL) HICR_THROW_LOGIC("The passed memory slot is not supported by this backend\n");

    // Locking mutex
    return m->trylock();
  }

  __USED__ inline void releaseGlobalLockImpl(HiCR::L0::GlobalMemorySlot *memorySlot) override
  {
    // Getting up-casted pointer for the execution unit
    auto m = dynamic_cast<L0::GlobalMemorySlot *>(memorySlot);

    // Checking whether the execution unit passed is compatible with this backend
    if (m == NULL) HICR_THROW_LOGIC("The passed memory slot is not supported by this backend\n");

    // Locking mutex
    m->unlock();
  }
};

} // namespace L1

} // namespace sharedMemory

} // namespace backend

} // namespace HiCR
