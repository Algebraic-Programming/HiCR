/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file communicationManager.hpp
 * @brief This is the LPF communication backend implementation, which is currently tested with
 * the LPF implementation under https://github.com/Algebraic-Programming/LPF/tree/hicr
 * (e.g. commit #8dea881)
 * @author K. Dichev
 * @date 24/10/2023
 */

#pragma once

#include <cstring>
#include <set>
#include <lpf/collectives.h>
#include <lpf/core.h>
#include <hicr/core/L0/localMemorySlot.hpp>
#include <hicr/core/L1/communicationManager.hpp>
#include "../L0/globalMemorySlot.hpp"
#include "../L0/localMemorySlot.hpp"

namespace HiCR
{

namespace backend
{

namespace lpf
{

namespace L1
{

/**
 * Implementation of the HiCR LPF communication manager
 *
 * The only LPF engine currently of interest is the IB Verbs engine (see above for branch and hash)
 */
class CommunicationManager final : public HiCR::L1::CommunicationManager
{
  public:

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
  CommunicationManager(size_t size, size_t rank, lpf_t lpf)
    : HiCR::L1::CommunicationManager(),
      _size(size),
      _rank(rank),
      _lpf(lpf),
      _localSwap(0ULL),
      _localSwapSlot(LPF_INVALID_MEMSLOT)
  {}

  private:

  const size_t _size;
  const size_t _rank;
  const lpf_t  _lpf;
  /**
   * localSwap is the varable holding the value we
   * compare against the global value at globalSwap
   */
  uint64_t _localSwap;
  /**
   * localSwapSlot is the LPF slot representing localSwap var
   */
  lpf_memslot_t _localSwapSlot;
  /**
   * globalSwap is a variable stored everywhere,
   * but at one of the ranks (a root) it gets globally advertised
   */
  // uint64_t globalSwap = 0ULL;
  /**
   * globalSwapSlot is the global LPF slot representing globalSlot var
   */
  // lpf_memslot_t globalSwapSlot = LPF_INVALID_MEMSLOT;

  /**
   * Map of global slot id and MPI windows
   */

  std::shared_ptr<HiCR::L0::GlobalMemorySlot> getGlobalMemorySlotImpl(HiCR::L0::GlobalMemorySlot::tag_t tag, HiCR::L0::GlobalMemorySlot::globalKey_t globalKey) override
  {
    return nullptr;
  }

  __INLINE__ void exchangeGlobalMemorySlotsImpl(HiCR::L0::GlobalMemorySlot::tag_t tag, const std::vector<globalKeyMemorySlotPair_t> &memorySlots) override
  {
    // Obtaining local slots to exchange
    size_t localSlotCount = memorySlots.size();
    /**
     * The following block is basically a verbose allgather from tests/functional/c99
     * in LPF repo
     */

    lpf_coll_t          coll;
    std::vector<size_t> globalSlotCounts(_size);
    for (size_t i = 0; i < _size; i++) globalSlotCounts[i] = 0;
    lpf_memslot_t src_slot                = LPF_INVALID_MEMSLOT;
    lpf_memslot_t dst_slot                = LPF_INVALID_MEMSLOT;
    lpf_memslot_t slot_local_sizes        = LPF_INVALID_MEMSLOT;
    lpf_memslot_t slot_local_keys         = LPF_INVALID_MEMSLOT;
    lpf_memslot_t slot_global_sizes       = LPF_INVALID_MEMSLOT;
    lpf_memslot_t slot_global_keys        = LPF_INVALID_MEMSLOT;
    lpf_memslot_t slot_local_process_ids  = LPF_INVALID_MEMSLOT;
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
    for (size_t i = 0; i < _size; i++) globalSlotCount += globalSlotCounts[i];

    std::vector<size_t> globalSlotCountsInBytes(_size);
    for (size_t i = 0; i < _size; i++) globalSlotCountsInBytes[i] = globalSlotCounts[i] * sizeof(size_t);

    // globalSlotSizes will hold exactly the union of all slot sizes at
    // each process (zero or more) to become global.

    std::vector<size_t>                                  localSlotSizes(localSlotCount);
    std::vector<HiCR::L0::GlobalMemorySlot::globalKey_t> localSlotKeys(localSlotCount);
    std::vector<size_t>                                  localSlotProcessId(localSlotCount);
    std::vector<size_t>                                  globalSlotSizes(globalSlotCount);
    std::vector<size_t>                                  globalSwapSlotSizes(globalSlotCount);
    std::vector<HiCR::L0::GlobalMemorySlot::globalKey_t> globalSlotKeys(globalSlotCount);
    std::vector<size_t>                                  globalSlotProcessId(globalSlotCount);

    for (size_t i = 0; i < localSlotCount; i++)
    {
      const auto key        = memorySlots[i].first;
      const auto memorySlot = memorySlots[i].second;
      localSlotSizes[i]     = memorySlot->getSize();
      localSlotKeys[i]      = key;
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

    CHECK(lpf_register_local(_lpf, &_localSwap, sizeof(uint64_t), &_localSwapSlot));

    size_t localPointerPos = 0;
    for (size_t i = 0; i < globalSlotCount; i++)
    {
      // If the rank associated with this slot is remote, don't store the pointer, otherwise store it.
      void                                      *globalSlotPointer     = nullptr;
      void                                      *globalSwapSlotPointer = nullptr;
      std::shared_ptr<HiCR::L0::LocalMemorySlot> globalSourceSlot      = nullptr;

      globalSwapSlotSizes[i] = sizeof(uint64_t);
      // If the slot is remote, do not specify any local size assigned to it
      if (globalSlotProcessId[i] != _rank)
      {
        globalSlotSizes[i]     = 0;
        globalSwapSlotSizes[i] = 0;
      }

      lpf_memslot_t                             newSlot       = LPF_INVALID_MEMSLOT;
      lpf_memslot_t                             swapValueSlot = LPF_INVALID_MEMSLOT;
      std::shared_ptr<lpf::L0::LocalMemorySlot> localSlot;
      // If it's local, then assign the local information to it
      if (globalSlotProcessId[i] == _rank)
      {
        auto memorySlot       = memorySlots[localPointerPos++].second;
        globalSlotPointer     = memorySlot->getPointer();
        localSlot             = (dynamic_pointer_cast<lpf::L0::LocalMemorySlot>(memorySlot));
        globalSwapSlotPointer = localSlot->getLPFSwapPointer();
        globalSourceSlot      = memorySlot;
      }
      // Registering with the LPF library
      CHECK(lpf_register_global(_lpf, globalSlotPointer, globalSlotSizes[i], &newSlot));
      CHECK(lpf_register_global(_lpf, globalSwapSlotPointer, globalSwapSlotSizes[i], &swapValueSlot));
      // Synchronizing with others
      CHECK(lpf_sync(_lpf, LPF_SYNC_DEFAULT));

      // Make sure the newly promoted slot points to the new
      // lpf_memslot_t reference -- otherwise querying the local slot
      // would yield incorrect results
      if (globalSlotProcessId[i] == _rank) { localSlot->setLPFSlot(newSlot); }

      // Creating new memory slot object
      auto memorySlot = std::make_shared<lpf::L0::GlobalMemorySlot>(globalSlotProcessId[i], newSlot, swapValueSlot, tag, globalSlotKeys[i], globalSourceSlot);

      // Finally, registering the new global memory slot
      registerGlobalMemorySlot(memorySlot);
    }
  }

  __INLINE__ void destroyGlobalMemorySlotsCollectiveImpl(HiCR::L0::GlobalMemorySlot::tag_t tag)
  {
    size_t localDestroySlotsCount = _globalMemorySlotsToDestroyPerTag[tag].size();

    // Allgather the global slot to destroy counts
    std::vector<size_t> globalDestroySlotCounts(_size);
    for (size_t i = 0; i < _size; i++) globalDestroySlotCounts[i] = 0;
    lpf_coll_t    coll;
    lpf_memslot_t src_slot = LPF_INVALID_MEMSLOT;
    lpf_memslot_t dst_slot = LPF_INVALID_MEMSLOT;

    CHECK(lpf_register_global(_lpf, &localDestroySlotsCount, sizeof(size_t), &src_slot));
    CHECK(lpf_register_global(_lpf, globalDestroySlotCounts.data(), sizeof(size_t) * _size, &dst_slot));
    CHECK(lpf_sync(_lpf, LPF_SYNC_DEFAULT));

    CHECK(lpf_collectives_init(_lpf, _rank, _size, 1, 0, sizeof(size_t) * _size, &coll));
    CHECK(lpf_allgather(coll, src_slot, dst_slot, sizeof(size_t), false /* exclude myself */));
    CHECK(lpf_sync(_lpf, LPF_SYNC_DEFAULT));
    CHECK(lpf_collectives_destroy(coll));
    CHECK(lpf_deregister(_lpf, src_slot));
    CHECK(lpf_deregister(_lpf, dst_slot));
    // End allgather slots to destroy counts block

    size_t globalDestroySlotTotalCount = 0;
    for (size_t i = 0; i < _size; i++) globalDestroySlotTotalCount += globalDestroySlotCounts[i];

    // We need to destroy both the slot and the swap slot
    localDestroySlotsCount *= 2lu;
    globalDestroySlotTotalCount *= 2lu;

    std::vector<size_t> globalDestroySlotCountsInBytes(_size);
    for (size_t i = 0; i < _size; i++) globalDestroySlotCountsInBytes[i] = globalDestroySlotCounts[i] * 2 * sizeof(size_t);

    // Allgather the global slot keys: Here we use the actual slot_t as the key! (contrary to MPI)
    // We need both the slot and the swap slot IDs
    std::vector<lpf_memslot_t> localDestroySlotIds(localDestroySlotsCount);
    std::vector<lpf_memslot_t> globalDestroySlotIds(globalDestroySlotTotalCount);

    // Filling in the local IDs storage
    size_t i = 0;
    for (auto slot : _globalMemorySlotsToDestroyPerTag[tag])
    {
      const auto memorySlot = std::dynamic_pointer_cast<HiCR::backend::lpf::L0::GlobalMemorySlot>(slot);
      if (memorySlot.get() == nullptr) HICR_THROW_FATAL("Trying to use LPF to destroy a non-LPF global slot");
      localDestroySlotIds[i++] = memorySlot->getLPFSlot();
      localDestroySlotIds[i++] = memorySlot->getLPFSwapSlot();
    }

    lpf_memslot_t slot_local_ids  = LPF_INVALID_MEMSLOT;
    lpf_memslot_t slot_global_ids = LPF_INVALID_MEMSLOT;

    // Prepare the relevant slots
    CHECK(lpf_register_local(_lpf, localDestroySlotIds.data(), localDestroySlotsCount * sizeof(lpf_memslot_t), &slot_local_ids));
    CHECK(lpf_register_global(_lpf, globalDestroySlotIds.data(), globalDestroySlotTotalCount * sizeof(lpf_memslot_t), &slot_global_ids));
    CHECK(lpf_sync(_lpf, LPF_SYNC_DEFAULT));

    // Execute the allgatherv
    CHECK(lpf_collectives_init(_lpf, _rank, _size, 1, 0, sizeof(lpf_memslot_t) * globalDestroySlotTotalCount, &coll));
    CHECK(lpf_sync(_lpf, LPF_SYNC_DEFAULT));
    CHECK(lpf_allgatherv(coll, slot_local_ids, slot_global_ids, globalDestroySlotCountsInBytes.data(), false /* exclude myself */));
    CHECK(lpf_sync(_lpf, LPF_SYNC_DEFAULT));
    CHECK(lpf_collectives_destroy(coll));
    CHECK(lpf_deregister(_lpf, slot_local_ids));
    CHECK(lpf_deregister(_lpf, slot_global_ids));
    // End allgather global slot IDs block

    // Deduplicate the global slot keys
    std::set<lpf_memslot_t> globalDestroySlotIdsSet(globalDestroySlotIds.begin(), globalDestroySlotIds.end());

    // Now we can iterate over the global slots to destroy one by one
    for (auto id : globalDestroySlotIdsSet) lpf_deregister(_lpf, id);
  }

  /**
   * Deletes a global memory slot from the backend. This operation is collective.
   * Attempting to access the global memory slot after this operation will result in undefined behavior.
   *
   * \param[in] memorySlotPtr Memory slot to destroy.
   */
  __INLINE__ void destroyGlobalMemorySlotImpl(std::shared_ptr<HiCR::L0::GlobalMemorySlot> memorySlotPtr) override
  {
    // Getting up-casted pointer for the execution unit
    auto memorySlot = dynamic_pointer_cast<lpf::L0::GlobalMemorySlot>(memorySlotPtr);

    // Checking whether the execution unit passed is compatible with this backend
    if (memorySlot == NULL) HICR_THROW_LOGIC("The memory slot is not supported by this backend\n");

    // Deregistering from LPF
    lpf_deregister(_lpf, memorySlot->getLPFSlot());
    lpf_deregister(_lpf, memorySlot->getLPFSwapSlot());
  }

  __INLINE__ void memcpyImpl(std::shared_ptr<HiCR::L0::LocalMemorySlot>  destination,
                             size_t                                      dst_offset,
                             std::shared_ptr<HiCR::L0::GlobalMemorySlot> source,
                             size_t                                      src_offset,
                             size_t                                      size) override
  {
    // Getting up-casted pointer
    auto src = dynamic_pointer_cast<lpf::L0::GlobalMemorySlot>(source);

    // Checking whether the execution unit passed is compatible with this backend
    if (source == NULL) HICR_THROW_LOGIC("The passed source memory slot is not supported by this backend\n");

    // Getting up-casted pointer
    auto dest = dynamic_pointer_cast<lpf::L0::LocalMemorySlot>(destination);

    // Checking whether the execution unit passed is compatible with this backend
    if (dest == NULL) HICR_THROW_LOGIC("The passed destination memory slot is not supported by this backend\n");

    // Getting internal lpf slots
    lpf_memslot_t srcSlot = src->getLPFSlot();
    lpf_memslot_t dstSlot = dest->getLPFSlot();

    // Getting remote rank
    auto remoteRank = src->getRank();

    // Perform the get operation
    lpf_get(_lpf, remoteRank, srcSlot, src_offset, dstSlot, dst_offset, size, LPF_MSG_DEFAULT);
  }

  __INLINE__ void memcpyImpl(std::shared_ptr<HiCR::L0::GlobalMemorySlot> destination,
                             size_t                                      dst_offset,
                             std::shared_ptr<HiCR::L0::LocalMemorySlot>  source,
                             size_t                                      src_offset,
                             size_t                                      size) override
  {
    // Getting up-casted pointer
    auto src = dynamic_pointer_cast<lpf::L0::LocalMemorySlot>(source);

    // Checking whether the execution unit passed is compatible with this backend
    if (src == NULL) HICR_THROW_LOGIC("The passed source memory slot is not supported by this backend\n");

    // Getting up-casted pointer
    auto dest = dynamic_pointer_cast<lpf::L0::GlobalMemorySlot>(destination);

    // Checking whether the execution unit passed is compatible with this backend
    if (dest == NULL) HICR_THROW_LOGIC("The passed destination memory slot is not supported by this backend\n");

    // Calculating pointers
    lpf_memslot_t dstSlot = dest->getLPFSlot();
    lpf_memslot_t srcSlot = src->getLPFSlot();

    // Getting remote rank
    auto remoteRank = dest->getRank();

    // Perform the put operation
    lpf_put(_lpf, srcSlot, src_offset, remoteRank, dstSlot, dst_offset, size, LPF_MSG_DEFAULT);
  }

  /**
   * Implementation of the fence operation for the LPF backend. For every single window corresponding.
   * Tags are currently ignored.
   * @param[in] tag Tags used as filter to decide which slots to fence against
   * \todo: Implement tags in LPF !!!
   */
  __INLINE__ void fenceImpl(HiCR::L0::GlobalMemorySlot::tag_t tag) override
  {
    CHECK(lpf_sync(_lpf, LPF_SYNC_DEFAULT));

    // Call the slot destrction collective routine
    destroyGlobalMemorySlotsCollectiveImpl(tag);
  }

  /**
   * gets global memory slots associated with a tag
   * \param[in] tag Tag associated with a set of memory slots
   * \return The global memory slots associated with this tag
   */
  __INLINE__ globalKeyToMemorySlotMap_t getGlobalMemorySlots(L0::GlobalMemorySlot::tag_t tag)
  {
    // If the requested tag and key are not found, return empty storage
    if (_globalMemorySlotTagKeyMap.contains(tag) == false)
      HICR_THROW_LOGIC("getGlobalMemorySlots: Requesting a global memory slot for a tag (%lu) that has not been registered.", tag);
    globalKeyToMemorySlotMap_t slotsForTag = _globalMemorySlotTagKeyMap.at(tag);
    return slotsForTag;
  }

  /**
   * Implementaion of fence on a specific local memory slot for the LPF backend
   * \param[in] hicrSlot local memory slot to fence on
   */
  __INLINE__ void fenceImpl(std::shared_ptr<HiCR::L0::LocalMemorySlot> hicrSlot)
  {
    auto          memorySlot = dynamic_pointer_cast<lpf::L0::LocalMemorySlot>(hicrSlot);
    lpf_memslot_t slot       = memorySlot->getLPFSlot();
    CHECK(lpf_sync_per_slot(_lpf, LPF_SYNC_DEFAULT, slot));
    updateMessagesRecv(memorySlot);
    updateMessagesSent(memorySlot);
  }

  /**
   * Implementaion of fence on a specific local memory slot for the LPF backend
   * \param[in] slot local memory slot to fence on
   * \param[in] expectedSent message count to be blockingly sent out on this slot
   * \param[in] expectedRcvd message count to be blockingly received on this slot
   */
  __INLINE__ void fenceImpl(std::shared_ptr<HiCR::L0::LocalMemorySlot> slot, size_t expectedSent, size_t expectedRcvd)
  {
    auto          memSlot = dynamic_pointer_cast<lpf::L0::LocalMemorySlot>(slot);
    lpf_memslot_t lpfSlot = memSlot->getLPFSlot();
    if (lpfSlot == LPF_INVALID_MEMSLOT) { HICR_THROW_LOGIC("This slot is not registered with LPF!"); }
    CHECK(lpf_counting_sync_per_slot(_lpf, LPF_SYNC_DEFAULT, lpfSlot, expectedSent, expectedRcvd));
    memSlot->setMessagesRecv(memSlot->getMessagesRecv() + expectedRcvd);
    memSlot->setMessagesSent(memSlot->getMessagesSent() + expectedSent);
  }

  /**
   * This method passively updates the received message count
   * on a memory slot from LPF. It does not ask LPF to do any
   * additional polling on messages.
   * \param[out] memorySlot HiCr memory slot to update
   */
  __INLINE__ void updateMessagesRecv(std::shared_ptr<HiCR::L0::LocalMemorySlot> memorySlot)
  {
    size_t        msg_cnt;
    auto          memSlot = dynamic_pointer_cast<lpf::L0::LocalMemorySlot>(memorySlot);
    lpf_memslot_t lpfSlot = memSlot->getLPFSlot();
    lpf_get_rcvd_msg_count_per_slot(_lpf, &msg_cnt, lpfSlot);
    memSlot->setMessagesRecv(msg_cnt);
  }

  /**
   * This method passively updates the sent message count
   * on a memory slot from LPF. It does not ask LPF to do any
   * additional polling on messages.
   * \param[out] memorySlot HiCr memory slot to update
   */
  __INLINE__ void updateMessagesSent(std::shared_ptr<HiCR::L0::LocalMemorySlot> memorySlot)
  {
    size_t        msg_cnt;
    auto          memSlot = dynamic_pointer_cast<lpf::L0::LocalMemorySlot>(memorySlot);
    lpf_memslot_t lpfSlot = memSlot->getLPFSlot();
    lpf_get_sent_msg_count_per_slot(_lpf, &msg_cnt, lpfSlot);
    memSlot->setMessagesSent(msg_cnt);
  }

  __INLINE__ void queryMemorySlotUpdatesImpl(std::shared_ptr<HiCR::L0::LocalMemorySlot> memorySlot) override { fenceImpl(memorySlot); }

  __INLINE__ void flushSent() override { lpf_flush_sent(_lpf); }
  __INLINE__ void flushReceived() override { lpf_flush_received(_lpf); }

  __INLINE__ bool acquireGlobalLockImpl(std::shared_ptr<HiCR::L0::GlobalMemorySlot> memorySlot) override
  {
    auto          hicrSlot    = dynamic_pointer_cast<lpf::L0::GlobalMemorySlot>(memorySlot);
    lpf_memslot_t lpfSwapSlot = hicrSlot->getLPFSwapSlot();
    auto          slotRank    = hicrSlot->getRank();
    CHECK(lpf_lock_slot(_lpf, _localSwapSlot, 0, slotRank, lpfSwapSlot, 0, sizeof(uint64_t), LPF_MSG_DEFAULT));
    return true;
  }

  __INLINE__ void releaseGlobalLockImpl(std::shared_ptr<HiCR::L0::GlobalMemorySlot> memorySlot) override
  {
    auto          hicrSlot    = dynamic_pointer_cast<lpf::L0::GlobalMemorySlot>(memorySlot);
    auto          slotRank    = hicrSlot->getRank();
    lpf_memslot_t lpfSwapSlot = hicrSlot->getLPFSwapSlot();
    CHECK(lpf_unlock_slot(_lpf, _localSwapSlot, 0, slotRank, lpfSwapSlot, 0, sizeof(uint64_t), LPF_MSG_DEFAULT));
  }
};

} // namespace L1

} // namespace lpf

} // namespace backend

} // namespace HiCR
