/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file memoryManager.hpp
 * @brief This is the LPF backend implementation, which is currently tested with
 * the LPF implementation under https://github.com/Algebraic-Programming/LPF/tree/hicr
 * (e.g. commit #8dea881)
 * @author K. Dichev
 * @date 24/10/2023
 */

#pragma once

/**
 * #DEFAULT_MEMSLOTS The memory slots used by LPF
 * in lpf_resize_memory_register . This value is currently
 * guessed as sufficiently large for a program
 */
#define DEFAULT_MEMSLOTS 10 + _size

/**
 * #DEFAULT_MSGSLOTS The message slots used by LPF
 * in lpf_resize_message_queue . This value is currently
 * guessed as sufficiently large for a program
 */
#define DEFAULT_MSGSLOTS 10 * _size

/**
 * #CHECK(f...) Checks if an LPF function returns LPF_SUCCESS, else
 * it prints an error message
 */
#define CHECK(f...)  if (f != LPF_SUCCESS) HICR_THROW_RUNTIME("LPF Backend Error: '%s'", #f);

#include <cstring>
#include <lpf/collectives.h>
#include <lpf/core.h>
#include <hicr/L1/memoryManager.hpp>
#include <backends/sequential/L1/memoryManager.hpp>
#include <backends/lpf/L0/memorySlot.hpp>

namespace HiCR
{

namespace backend
{

namespace lpf
{

namespace L1
{

/**
 * This macro represents an identifier for the default system-wide memory space in this backend
 */
#define _BACKEND_LPF_DEFAULT_MEMORY_SPACE_ID 0

/**
 * Implementation of the HiCR LPF backend
 *
 * The only LPF engine currently of interest is the IB Verbs engine (see above for branch and hash)
 */
class MemoryManager final : public HiCR::L1::MemoryManager
{
  private:

  const size_t _size;
  const size_t _rank;
  const lpf_t _lpf;

  /**
   * Map of global slot id and MPI windows
   */

  public:

  /**
   * This function returns the available allocatable size in the current system RAM
   *
   * @param[in] memorySpace Always zero, represents the system's RAM
   * @return The allocatable size within the system
   */
  size_t getMemorySpaceSizeImpl(const memorySpaceId_t memorySpace) const
  {
    if (memorySpace != _BACKEND_LPF_DEFAULT_MEMORY_SPACE_ID)
      HICR_THROW_RUNTIME("This backend does not support multiple memory spaces. Provided: %lu, Expected: %lu", memorySpace, (memorySpaceId_t)_BACKEND_LPF_DEFAULT_MEMORY_SPACE_ID);

    return sequential::L1::MemoryManager::getTotalSystemMemory();
  }

  /**
   * A map from a HiCR slot ID to the initial message count. This count is unlikely
   * to be zero, as slots get internally reused and reassigned in LPF.
   * It is important to know the initial count per slot so as to avoid
   * incrementing the messagesRecv at the beginning without need.
   */
  std::map<L0::MemorySlot, size_t> initMsgCnt;

  /**
   * Constructor of the LPF memory manager
   * @param[in] size Communicator size
   * @param[in] rank Process rank
   * @param[in] lpf LPF context
   * \note The decision to resize memory register in the constructor
   * is because this call requires lpf_sync to become effective.
   * This makes it almost impossible to do local memory registrations with LPF.
   * On the other hand, the resize message queue could also be locally
   * made, and placed elsewhere.
   */
  MemoryManager(size_t size, size_t rank, lpf_t lpf) : HiCR::L1::MemoryManager(), _size(size), _rank(rank), _lpf(lpf)
  {
    const size_t msgslots = DEFAULT_MSGSLOTS;
    const size_t memslots = DEFAULT_MEMSLOTS;
    CHECK(lpf_resize_message_queue(_lpf, msgslots));
    CHECK(lpf_resize_memory_register(_lpf, memslots));
    CHECK(lpf_sync(_lpf, LPF_SYNC_DEFAULT));
  }

  /**
   * Exchanges memory slots among different local instances of HiCR to enable global (remote) communication
   *
   * \param[in] tag Identifies a particular subset of global memory slots
   * \param[in] memorySlots Array of local memory slots to make globally accessible
   */
  __USED__ inline void exchangeGlobalMemorySlotsImpl(const HiCR::L0::MemorySlot::tag_t tag, const std::vector<globalKeyMemorySlotPair_t> &memorySlots) override
  {
    // Obtaining local slots to exchange
    size_t localSlotCount = memorySlots.size();
    /**
     * The following block is basically a verbose allgather from tests/functional/c99
     * in LPF repo
     */

    lpf_coll_t coll;
    std::vector<size_t> globalSlotCounts(_size);
    for (size_t i = 0; i < _size; i++) globalSlotCounts[i] = 0;
    lpf_memslot_t src_slot = LPF_INVALID_MEMSLOT;
    lpf_memslot_t dst_slot = LPF_INVALID_MEMSLOT;
    lpf_memslot_t slot_local_sizes = LPF_INVALID_MEMSLOT;
    lpf_memslot_t slot_local_keys = LPF_INVALID_MEMSLOT;
    lpf_memslot_t slot_global_sizes = LPF_INVALID_MEMSLOT;
    lpf_memslot_t slot_global_keys = LPF_INVALID_MEMSLOT;
    lpf_memslot_t slot_local_process_ids = LPF_INVALID_MEMSLOT;
    lpf_memslot_t slot_global_process_ids = LPF_INVALID_MEMSLOT;

    CHECK(lpf_register_global(_lpf, &localSlotCount, sizeof(size_t), &src_slot));
    CHECK(lpf_register_global(_lpf, globalSlotCounts.data(), sizeof(size_t) * _size, &dst_slot));
    CHECK(lpf_collectives_init(_lpf, _rank, _size, 1, 0, sizeof(size_t) * _size, &coll));
    CHECK(lpf_allgather(coll, src_slot, dst_slot, sizeof(size_t), false));

    CHECK(lpf_sync(_lpf, LPF_SYNC_DEFAULT));
    CHECK(lpf_collectives_destroy(coll));
    CHECK(lpf_deregister(_lpf, src_slot));
    CHECK(lpf_deregister(_lpf, dst_slot));
    // end allgather block

    size_t globalSlotCount = 0;
    for (size_t i = 0; i < _size; i++)
    {
      globalSlotCount += globalSlotCounts[i];
    }
    std::vector<size_t> globalSlotCountsInBytes(globalSlotCount);
    for (size_t i = 0; i < globalSlotCount; i++)
    {
      globalSlotCountsInBytes[i] = globalSlotCounts[i] * sizeof(size_t);
    }

    // globalSlotSizes will hold exactly the union of all slot sizes at
    // each process (zero or more) to become global.

    std::vector<size_t> localSlotSizes(localSlotCount);
    std::vector<HiCR::L0::MemorySlot::globalKey_t> localSlotKeys(localSlotCount);
    std::vector<size_t> localSlotProcessId(localSlotCount);
    std::vector<size_t> globalSlotSizes(globalSlotCount);
    std::vector<HiCR::L0::MemorySlot::globalKey_t> globalSlotKeys(globalSlotCount);
    std::vector<void *> globalSlotPointers(globalSlotCount);
    std::vector<size_t> globalSlotProcessId(globalSlotCount);

    for (size_t i = 0; i < localSlotCount; i++)
    {
      const auto key = memorySlots[i].first;
      const auto memorySlot = memorySlots[i].second;
      localSlotSizes[i] = memorySlot->getSize();
      localSlotKeys[i] = key;
      localSlotProcessId[i] = _rank;
    }

    CHECK(lpf_register_local(_lpf, localSlotSizes.data(), localSlotCount * sizeof(size_t), &slot_local_sizes));
    CHECK(lpf_register_global(_lpf, globalSlotSizes.data(), globalSlotCount * sizeof(size_t), &slot_global_sizes));
    CHECK(lpf_sync(_lpf, LPF_SYNC_DEFAULT));
    // start allgatherv for global slot counts in bytes
    CHECK(lpf_collectives_init(_lpf, _rank, _size, 2 /* will call gatherv 2 times */, 0, sizeof(size_t) * globalSlotCount, &coll));
    CHECK(lpf_allgatherv(coll, slot_local_sizes, slot_global_sizes, globalSlotCountsInBytes.data(), false /*exclude myself*/));
    CHECK(lpf_sync(_lpf, LPF_SYNC_DEFAULT));
    CHECK(lpf_register_local(_lpf, localSlotProcessId.data(), localSlotCount * sizeof(size_t), &slot_local_process_ids));
    CHECK(lpf_register_global(_lpf, globalSlotProcessId.data(), globalSlotCount * sizeof(size_t), &slot_global_process_ids));
    CHECK(lpf_sync(_lpf, LPF_SYNC_DEFAULT));
    // start allgatherv for process IDs assigned to each global slot
    CHECK(lpf_allgatherv(coll, slot_local_process_ids, slot_global_process_ids, globalSlotCountsInBytes.data(), false /*exclude myself*/));
    CHECK(lpf_sync(_lpf, LPF_SYNC_DEFAULT));
    CHECK(lpf_collectives_destroy(coll));
    CHECK(lpf_deregister(_lpf, slot_local_sizes));
    CHECK(lpf_deregister(_lpf, slot_global_sizes));
    CHECK(lpf_deregister(_lpf, slot_local_process_ids));
    CHECK(lpf_deregister(_lpf, slot_global_process_ids));

    CHECK(lpf_register_local(_lpf, localSlotKeys.data(), localSlotCount * sizeof(size_t), &slot_local_keys));
    CHECK(lpf_register_global(_lpf, globalSlotKeys.data(), globalSlotCount * sizeof(size_t), &slot_global_keys));
    CHECK(lpf_sync(_lpf, LPF_SYNC_DEFAULT));

    CHECK(lpf_collectives_init(_lpf, _rank, _size, 1, 0, sizeof(size_t) * globalSlotCount, &coll));
    CHECK(lpf_allgatherv(coll, slot_local_keys, slot_global_keys, globalSlotCountsInBytes.data(), false /*exclude myself*/));
    CHECK(lpf_sync(_lpf, LPF_SYNC_DEFAULT));
    CHECK(lpf_collectives_destroy(coll));
    CHECK(lpf_deregister(_lpf, slot_local_keys));
    CHECK(lpf_deregister(_lpf, slot_global_keys));

    size_t localPointerPos = 0;
    for (size_t i = 0; i < globalSlotCount; i++)
    {
      // If the rank associated with this slot is remote, don't store the pointer, otherwise store it.

      // Memory for this slot not yet allocated or registered
      if (globalSlotProcessId[i] != _rank)
      {
        globalSlotPointers[i] = new char[globalSlotSizes[i]]; // NULL;
        // optionally initialize here?
      }
      // Memory already allocated and locally registered
      else
      {
        // deregister locally as it will be registered globally

        lpf::L0::MemorySlot *memorySlot = static_cast<lpf::L0::MemorySlot *>(memorySlots[localPointerPos++].second);
        lpf_deregister(_lpf, memorySlot->getLPFSlot());
        globalSlotPointers[i] = memorySlot->getPointer();
        // optionally initialize here?
      }

      lpf_memslot_t newSlot = LPF_INVALID_MEMSLOT;
      CHECK(lpf_register_global(_lpf, globalSlotPointers[i], globalSlotSizes[i], &newSlot));

      // Creating new memory slot object
      auto memorySlot = new lpf::L0::MemorySlot(
        globalSlotProcessId[i],
        newSlot,
        globalSlotPointers[i],
        globalSlotSizes[i],
        tag,
        globalSlotKeys[i]);

      CHECK(lpf_sync(_lpf, LPF_SYNC_DEFAULT));

      size_t msg_cnt;
      lpf_get_rcvd_msg_count_per_slot(_lpf, &msg_cnt, memorySlot->getLPFSlot());
      initMsgCnt[*memorySlot] = msg_cnt;

      registerGlobalMemorySlot(memorySlot);
    }
  }

  __USED__ inline void memcpyImpl(HiCR::L0::MemorySlot *destinationSlotPtr, const size_t dstOffset, HiCR::L0::MemorySlot *sourceSlotPtr, const size_t srcOffset, const size_t size) override
  {
    /**
     * For a put:
     * 1. The destination slot needs to be global
     * 2. The source slot may be either local or global
     * For a get:
     * 1. The destination slot may be either local or global
     * 2. The source slot needs to be global
     *
     * -> These principles determine the search in the hash maps
     *  and the error handling
     */

    // Getting up-casted pointer for the execution unit
    auto destination = dynamic_cast<L0::MemorySlot *>(destinationSlotPtr);

    // Checking whether the execution unit passed is compatible with this backend
    if (destination == NULL) HICR_THROW_LOGIC("The passed destination memory slot is not supported by this backend\n");

    // Getting up-casted pointer for the execution unit
    auto source = dynamic_cast<L0::MemorySlot *>(sourceSlotPtr);

    // Checking whether the execution unit passed is compatible with this backend
    if (source == NULL) HICR_THROW_LOGIC("The passed source memory slot is not supported by this backend\n");

    // Getting ranks for the involved processes
    const auto sourceRank = source->getRank();
    const auto destinationRank = destination->getRank();

    bool isSourceRemote = sourceRank != _rank;
    bool isDestinationRemote = destinationRank != _rank;

    // Checking whether source and/or remote are remote

    // Sanity checks
    if (isSourceRemote && isDestinationRemote) HICR_THROW_LOGIC("Trying to use LPF backend perform a remote to remote copy between slots");

    // if (isSourceRemote && isDestinationGlobalSlot == false) HICR_THROW_LOGIC("Trying to use MPI backend in remote operation to with a destination slot(%lu) that has not been registered as global.", destination);

    // Calculating pointers
    lpf_memslot_t dstSlot = destination->getLPFSlot();
    lpf_memslot_t srcSlot = source->getLPFSlot();

    // Perform a get if the source is remote and destination is local
    if (isSourceRemote && !isDestinationRemote)
    {
      lpf_get(_lpf, srcSlot, srcOffset, destination->getRank(), dstSlot, dstOffset, size, LPF_MSG_DEFAULT);
    }
    // Perform a put if source is local and destination is global
    else if (!isSourceRemote && isDestinationRemote)
    {
      lpf_put(_lpf, srcSlot, srcOffset, destination->getRank(), dstSlot, dstOffset, size, LPF_MSG_DEFAULT);
    }
  }

  /**
   * Implementation of the fence operation for the LPF backend. For every single window corresponding.
   * Tags are currently ignored.
   * @param[in] tag Tags used as filter to decide which slots to fence against
   * \todo: Implement tags in LPF !!!
   */
  __USED__ inline void fenceImpl(const HiCR::L0::MemorySlot::tag_t tag) override
  {
    lpf_sync(_lpf, LPF_SYNC_DEFAULT);
  }

  /**
   * Associates a pointer locally-allocated manually and creates a local memory slot with it
   *
   * \param[in] ptr Pointer to the local memory space
   * \param[in] size Size of the memory slot to register
   * \return A newly created memory slot
   */
  __USED__ inline HiCR::L0::MemorySlot *registerLocalMemorySlotImpl(void *const ptr, const size_t size) override
  {
    lpf_memslot_t lpfSlot = LPF_INVALID_MEMSLOT;
    auto rc = lpf_register_local(_lpf, ptr, size, &lpfSlot);
    if (rc != LPF_SUCCESS) HICR_THROW_RUNTIME("LPF Memory Manager: lpf_register_local failed");

    // Creating new memory slot object
    auto memorySlot = new L0::MemorySlot(_rank, lpfSlot, ptr, size);
    return memorySlot;
  }

  __USED__ inline void queryMemorySlotUpdatesImpl(HiCR::L0::MemorySlot *memorySlot) override
  {
    pullMessagesRecv(memorySlot);
  }

  __USED__ inline void deregisterGlobalMemorySlotImpl(HiCR::L0::MemorySlot *memorySlotPtr) override
  {
    // Getting up-casted pointer for the execution unit
    auto memorySlot = dynamic_cast<L0::MemorySlot *>(memorySlotPtr);

    // Checking whether the execution unit passed is compatible with this backend
    if (memorySlot == NULL) HICR_THROW_LOGIC("The memory slot is not supported by this backend\n");

    lpf_deregister(_lpf, memorySlot->getLPFSlot());
  }

  __USED__ inline void deregisterLocalMemorySlotImpl(HiCR::L0::MemorySlot *memorySlot) override
  {
    // Nothing to do here for this backend
  }

  __USED__ inline void freeLocalMemorySlotImpl(HiCR::L0::MemorySlot *memorySlot) override
  {
    // Getting memory slot pointer
    const auto pointer = memorySlot->getPointer();

    // Checking whether the pointer is valid
    if (pointer == NULL) HICR_THROW_RUNTIME("Invalid memory slot(s) provided. It either does not exist or represents a NULL pointer.");

    // Deallocating memory using MPI's free mechanism
    free(pointer);
  }

  __USED__ inline HiCR::L0::MemorySlot *allocateLocalMemorySlotImpl(const memorySpaceId_t memorySpace, const size_t size) override
  {
    if (memorySpace != _BACKEND_LPF_DEFAULT_MEMORY_SPACE_ID)
      HICR_THROW_RUNTIME("This backend does not support multiple memory spaces. Provided: %lu, Expected: %lu", memorySpace, (memorySpaceId_t)_BACKEND_LPF_DEFAULT_MEMORY_SPACE_ID);

    // Attempting to allocate the new memory slot
    auto ptr = malloc(size);

    // Creating and returning new memory slot
    return registerLocalMemorySlotImpl(ptr, size);
  }

  __USED__ inline memorySpaceList_t queryMemorySpacesImpl() override
  {
    // Only a single memory space is created
    return memorySpaceList_t({_BACKEND_LPF_DEFAULT_MEMORY_SPACE_ID});
  }
  //    memorySlotId_t allocateMemorySlot(const memorySpaceId_t memorySpaceId, const size_t size)  {
  //
  //        memorySlotId_t m = 0; return m;}

  /**
   * This function pulls the received message count via LPF
   * from an LPF IB Verbs slot and sets the messagesRecv field of
   * a HiCR memory slot
   * @param[inout] memorySlot whose messageRecv should be updated
   */
  __USED__ inline void pullMessagesRecv(HiCR::L0::MemorySlot *memorySlot)
  {
    size_t msg_cnt;
    lpf::L0::MemorySlot *memSlot = static_cast<lpf::L0::MemorySlot *>(memorySlot);
    lpf_memslot_t lpfSlot = memSlot->getLPFSlot();
    lpf_get_rcvd_msg_count_per_slot(_lpf, &msg_cnt, lpfSlot);
    for (size_t i = initMsgCnt[*memSlot] + memSlot->getMessagesRecv(); i < msg_cnt; i++)
      memSlot->increaseMessagesRecv();
  }

  __USED__ inline void flush() override
  {
    lpf_flush(_lpf);
  }

  __USED__ inline bool acquireGlobalLockImpl(HiCR::L0::MemorySlot *memorySlot) override
  {
    HICR_THROW_RUNTIME("Not yet implemented for this backend");
  }

  __USED__ inline void releaseGlobalLockImpl(HiCR::L0::MemorySlot *memorySlot) override
  {
    HICR_THROW_RUNTIME("Not yet implemented for this backend");
  }
};

} // namespace L1

} // namespace lpf

} // namespace backend

} // namespace HiCR
