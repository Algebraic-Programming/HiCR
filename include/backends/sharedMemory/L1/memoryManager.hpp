/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file memoryManager.hpp
 * @brief This file implements the memory manager support for the shared memory backend
 * @author S. M. Martin
 * @date 14/8/2023
 */

#pragma once

#include "hwloc.h"
#include "pthread.h"
#include <backends/sharedMemory/L0/memorySpace.hpp>
#include <backends/sharedMemory/L0/memorySlot.hpp>
#include <hicr/L1/memoryManager.hpp>

namespace HiCR
{

namespace backend
{

namespace sharedMemory
{

namespace L1
{

/**
 * Implementation of the SharedMemory/HWloc-based HiCR Shared Memory Backend.
 */
class MemoryManager final : public HiCR::L1::MemoryManager
{
  public:

  /**
   * Constructor for the memory manager class for the shared memory backend
   *
   * \param[in] topology An HWloc topology object that can be used to query the available memory resources
   * \param[in] fenceCount Specifies how many times a fence has to be called for it to release callers
   */
  MemoryManager(const hwloc_topology_t *topology, const size_t fenceCount = 1) : HiCR::L1::MemoryManager(), _topology{topology}
  {
    // Initializing barrier for fence operation
    pthread_barrier_init(&_barrier, NULL, fenceCount);

    // Initializing mutex object
    pthread_mutex_init(&_mutex, NULL);
  }

  /**
   * The constructor is employed to free memory required for hwloc
   */
  ~MemoryManager()
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
  L0::MemorySlot::binding_type _hwlocBindingRequested = L0::MemorySlot::binding_type::strict_binding;

  /**
   * Local processor and memory hierarchy topology, as detected by Hwloc
   */
  const hwloc_topology_t *const _topology;

  /**
   * Backend-internal implementation of the queryLocalMemorySlot function
   *
   * \param[in] memorySpaceId Memory space to allocate memory in
   * \param[in] size Size of the memory slot to create
   * \return The internal pointer associated to the local memory slot
   */
  __USED__ inline HiCR::L0::MemorySlot *allocateLocalMemorySlotImpl(HiCR::L0::MemorySpace* memorySpace, const size_t size) override
  {
        // Getting up-casted pointer for the MPI instance
    auto m = dynamic_cast<const L0::MemorySpace *>(memorySpace);

    // Checking whether the execution unit passed is compatible with this backend
    if (m == NULL) HICR_THROW_LOGIC("The passed memory space is not supported by this memory manager\n");

    // Getting binding type supported by the memory space
    const auto supportedBindingType = m->getSupportedBindingType();

    // Checking whether the operation requested is supported by the HWLoc on this memory space
    if (_hwlocBindingRequested > supportedBindingType) HICR_THROW_LOGIC("Requesting an allocation binding support level (%u) not supported by the operating system (HWLoc max support: %u)", _hwlocBindingRequested, supportedBindingType);

    // Getting memory space's HWLoc object to perform the operation with
    const auto hwlocObj = m->getHWLocObject();

    // Allocating memory in the reqested memory space
    void *ptr = NULL;
    if (supportedBindingType == L0::MemorySlot::binding_type::strict_binding) ptr = hwloc_alloc_membind(*_topology, size, hwlocObj->nodeset, HWLOC_MEMBIND_DEFAULT, HWLOC_MEMBIND_BYNODESET | HWLOC_MEMBIND_STRICT);
    if (supportedBindingType == L0::MemorySlot::binding_type::strict_non_binding) ptr = malloc(size);

    // Error checking
    if (ptr == NULL) HICR_THROW_RUNTIME("Could not allocate memory (size %lu) in the requested memory space", size);

    // Creating new memory slot object
    auto memorySlot = new L0::MemorySlot(supportedBindingType, ptr, size, memorySpace);

    // Assinging new entry in the memory slot map
    return memorySlot;
  }

  /**
   * Backend-internal implementation of the registerLocalMemorySlot function
   *
   * \param[in] ptr Pointer to the local memory space
   * \param[in] size Size of the memory slot to register
   * \return A newly created memory slot
   */
  __USED__ inline HiCR::L0::MemorySlot *registerLocalMemorySlotImpl(HiCR::L0::MemorySpace* memorySpace, void *const ptr, const size_t size) override
  {
    // Creating new memory slot object
    auto memorySlot = new L0::MemorySlot(L0::MemorySlot::binding_type::strict_non_binding, ptr, size, memorySpace);

    // Returning new memory slot pointer
    return memorySlot;
  }

  /**
   * Backend-internal implementation of the deregisterMemorySlot function
   *
   * \param[in] memorySlot Memory slot to deregister.
   */
  __USED__ inline void deregisterLocalMemorySlotImpl(HiCR::L0::MemorySlot *const memorySlot) override
  {
    // Nothing to do here
  }

  __USED__ inline void deregisterGlobalMemorySlotImpl(HiCR::L0::MemorySlot *memorySlot) override
  {
    // Nothing to do here
  }

  /**
   * Exchanges memory slots among different local instances of HiCR to enable global (remote) communication
   *
   * \param[in] tag Identifies a particular subset of global memory slots
   * \param[in] memorySlots Array of local memory slots to make globally accessible
   */
  __USED__ inline void exchangeGlobalMemorySlotsImpl(const HiCR::L0::MemorySlot::tag_t tag, const std::vector<globalKeyMemorySlotPair_t> &memorySlots) override
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
      auto globalMemorySlot = new L0::MemorySlot(L0::MemorySlot::binding_type::strict_non_binding, memorySlot->getPointer(), memorySlot->getSize(), NULL, tag, globalKey);

      // Registering memory slot
      registerGlobalMemorySlot(globalMemorySlot);
    }

    // Releasing mutex
    pthread_mutex_unlock(&_mutex);

    // Do not allow any thread to continue until the exchange is made
    barrier();
  }

  /**
   * Backend-internal implementation of the freeLocalMemorySlot function
   *
   * \param[in] memorySlot Local memory slot to free up. It becomes unusable after freeing.
   */
  __USED__ inline void freeLocalMemorySlotImpl(HiCR::L0::MemorySlot *memorySlot) override
  {
    // Getting up-casted pointer for the execution unit
    auto m = dynamic_cast<L0::MemorySlot *>(memorySlot);

    // Checking whether the execution unit passed is compatible with this backend
    if (m == NULL) HICR_THROW_LOGIC("The passed memory slot is not supported by this backend\n");

    // Getting memory slot info
    const auto memorySlotBindingType = m->getBindingType();
    const auto memorySlotPointer = m->getPointer();
    const auto memorySlotSize = m->getSize();

    // If using strict binding, use hwloc_free to properly unmap the memory binding
    if (memorySlotBindingType == L0::MemorySlot::binding_type::strict_binding)
    {
      // Freeing memory slot
      auto status = hwloc_free(*_topology, memorySlotPointer, memorySlotSize);

      // Error checking
      if (status != 0) HICR_THROW_RUNTIME("Could not free bound memory slot.");
    }

    // If using strict non binding, use system's free
    if (memorySlotBindingType == L0::MemorySlot::binding_type::strict_non_binding)
    {
      free(memorySlotPointer);
    }
  }

  /**
   * Backend-internal implementation of the queryMemorySlotUpdates function
   *
   * \param[in] memorySlot Memory slot to query updates for.
   */
  __USED__ inline void queryMemorySlotUpdatesImpl(HiCR::L0::MemorySlot *memorySlot) override
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
  __USED__ inline void fenceImpl(const HiCR::L0::MemorySlot::tag_t tag) override
  {
    barrier();
  }

  __USED__ inline void memcpyImpl(HiCR::L0::MemorySlot *destination, const size_t dst_offset, HiCR::L0::MemorySlot *source, const size_t src_offset, const size_t size) override
  {
    // Getting slot pointers
    const auto srcPtr = source->getPointer();
    const auto dstPtr = destination->getPointer();

    // Calculating actual offsets
    const auto actualSrcPtr = (void *)((uint8_t *)srcPtr + src_offset);
    const auto actualDstPtr = (void *)((uint8_t *)dstPtr + dst_offset);

    // Running the requested operation
    std::memcpy(actualDstPtr, actualSrcPtr, size);

    // Increasing message received/sent counters for memory slots
    source->increaseMessagesSent();
    destination->increaseMessagesRecv();
  }

  __USED__ inline bool acquireGlobalLockImpl(HiCR::L0::MemorySlot *memorySlot) override
  {
    // Getting up-casted pointer for the execution unit
    auto m = dynamic_cast<L0::MemorySlot *>(memorySlot);

    // Checking whether the execution unit passed is compatible with this backend
    if (m == NULL) HICR_THROW_LOGIC("The passed memory slot is not supported by this backend\n");

    // Locking mutex
    return m->trylock();
  }

  __USED__ inline void releaseGlobalLockImpl(HiCR::L0::MemorySlot *memorySlot) override
  {
    // Getting up-casted pointer for the execution unit
    auto m = dynamic_cast<L0::MemorySlot *>(memorySlot);

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
