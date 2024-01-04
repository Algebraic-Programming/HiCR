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

#include <hicr/L0/localMemorySlot.hpp>
#include <hicr/L1/communicationManager.hpp>
#include <backends/lpf/L0/globalMemorySlot.hpp>
#include <backends/lpf/L0/localMemorySlot.hpp>
#include <cstring>
#include <lpf/collectives.h>
#include <lpf/core.h>

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
  private:

  const size_t _size;
  const size_t _rank;
  const lpf_t _lpf;

  /**
   * Map of global slot id and MPI windows
   */

  public:

  /**
   * A map from a HiCR slot ID to the initial message count. This count is unlikely
   * to be zero, as slots get internally reused and reassigned in LPF.
   * It is important to know the initial count per slot so as to avoid
   * incrementing the messagesRecv at the beginning without need.
   */
  std::map<L0::GlobalMemorySlot*, size_t> initMsgCnt;

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
  CommunicationManager(size_t size, size_t rank, lpf_t lpf) : HiCR::L1::CommunicationManager(), _size(size), _rank(rank), _lpf(lpf) {}

  /**
   * Exchanges memory slots among different local instances of HiCR to enable global (remote) communication
   *
   * \param[in] tag Identifies a particular subset of global memory slots
   * \param[in] memorySlots Array of local memory slots to make globally accessible
   */
  __USED__ inline void exchangeGlobalMemorySlotsImpl(const HiCR::L0::GlobalMemorySlot::tag_t tag, const std::vector<globalKeyMemorySlotPair_t> &memorySlots) override
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
    std::vector<HiCR::L0::GlobalMemorySlot::globalKey_t> localSlotKeys(localSlotCount);
    std::vector<size_t> localSlotProcessId(localSlotCount);
    std::vector<size_t> globalSlotSizes(globalSlotCount);
    std::vector<HiCR::L0::GlobalMemorySlot::globalKey_t> globalSlotKeys(globalSlotCount);
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
      void* globalSlotPointer = nullptr;
      std::shared_ptr<HiCR::L0::LocalMemorySlot> globalSourceSlot = nullptr;

      // Memory for this slot not yet allocated or registered
      if (globalSlotProcessId[i] != _rank)
      {
        globalSlotSizes[i] = 0;
      }
      else
      {
        auto memorySlot = memorySlots[localPointerPos++].second;
        globalSlotPointer = memorySlot->getPointer();
        globalSourceSlot = memorySlot;
      }

      lpf_memslot_t newSlot = LPF_INVALID_MEMSLOT;
      CHECK(lpf_register_global(_lpf, globalSlotPointer, globalSlotSizes[i], &newSlot));

      // Creating new memory slot object
      auto memorySlot = std::make_shared<lpf::L0::GlobalMemorySlot>(
        globalSlotProcessId[i],
        newSlot,
        tag,
        globalSlotKeys[i],
        globalSourceSlot);

      CHECK(lpf_sync(_lpf, LPF_SYNC_DEFAULT));

      size_t msg_cnt;
      lpf_get_rcvd_msg_count_per_slot(_lpf, &msg_cnt, memorySlot->getLPFSlot());
      initMsgCnt[memorySlot.get()] = msg_cnt;

      registerGlobalMemorySlot(memorySlot);
    }
  }

  __USED__ inline void memcpyImpl(std::shared_ptr<HiCR::L0::LocalMemorySlot> destinationSlotPtr, const size_t dstOffset, std::shared_ptr<HiCR::L0::GlobalMemorySlot> sourceSlotPtr, const size_t srcOffset, const size_t size) override
  {
    // Getting up-casted pointer
    auto source = dynamic_cast<lpf::L0::GlobalMemorySlot *>(sourceSlotPtr.get());

    // Checking whether the execution unit passed is compatible with this backend
    if (source == NULL) HICR_THROW_LOGIC("The passed source memory slot is not supported by this backend\n");

    // Getting up-casted pointer
    auto dest = dynamic_cast<lpf::L0::LocalMemorySlot *>(destinationSlotPtr.get());

    // Checking whether the execution unit passed is compatible with this backend
    if (dest == NULL) HICR_THROW_LOGIC("The passed destination memory slot is not supported by this backend\n");

    // Getting internal lpf slots
    lpf_memslot_t srcSlot = source->getLPFSlot();
    lpf_memslot_t dstSlot = dest->getLPFSlot();

    // Getting remote rank
    auto remoteRank = source->getRank();

    // Perform the get operation
    lpf_get(_lpf, srcSlot, srcOffset, remoteRank, dstSlot, dstOffset, size, LPF_MSG_DEFAULT);
  }

  __USED__ inline void memcpyImpl(std::shared_ptr<HiCR::L0::GlobalMemorySlot> destinationSlotPtr, const size_t dstOffset, std::shared_ptr<HiCR::L0::LocalMemorySlot> sourceSlotPtr, const size_t srcOffset, const size_t size) override
  {
    // Getting up-casted pointer
    auto source = dynamic_cast<lpf::L0::LocalMemorySlot *>(sourceSlotPtr.get());

    // Checking whether the execution unit passed is compatible with this backend
    if (source == NULL) HICR_THROW_LOGIC("The passed source memory slot is not supported by this backend\n");

    // Getting up-casted pointer
    auto dest = dynamic_cast<lpf::L0::GlobalMemorySlot *>(destinationSlotPtr.get());

    // Checking whether the execution unit passed is compatible with this backend
    if (dest == NULL) HICR_THROW_LOGIC("The passed destination memory slot is not supported by this backend\n");

    // Calculating pointers
    lpf_memslot_t dstSlot = dest->getLPFSlot();
    lpf_memslot_t srcSlot = source->getLPFSlot();

    // Getting remote rank
    auto remoteRank = dest->getRank();

    // Perform the put operation
    lpf_put(_lpf, srcSlot, srcOffset, remoteRank, dstSlot, dstOffset, size, LPF_MSG_DEFAULT);
  }

  /**
   * Implementation of the fence operation for the LPF backend. For every single window corresponding.
   * Tags are currently ignored.
   * @param[in] tag Tags used as filter to decide which slots to fence against
   * \todo: Implement tags in LPF !!!
   */
  __USED__ inline void fenceImpl(const HiCR::L0::GlobalMemorySlot::tag_t tag) override
  {
    lpf_sync(_lpf, LPF_SYNC_DEFAULT);
  }

  __USED__ inline void queryMemorySlotUpdatesImpl(std::shared_ptr<HiCR::L0::GlobalMemorySlot> memorySlot) override
  {
    // Getting up-casted pointer
    auto slot = dynamic_cast<lpf::L0::GlobalMemorySlot *>(memorySlot.get());

    // Checking whether the execution unit passed is compatible with this backend
    if (slot == NULL) HICR_THROW_LOGIC("The passed memory slot is not supported by this backend\n");

    pullMessagesRecv(slot);
  }

  __USED__ inline void deregisterGlobalMemorySlotImpl(std::shared_ptr<HiCR::L0::GlobalMemorySlot> memorySlotPtr) override
  {
    // Getting up-casted pointer for the execution unit
    auto memorySlot = dynamic_cast<const lpf::L0::GlobalMemorySlot *>(memorySlotPtr.get());

    // Checking whether the execution unit passed is compatible with this backend
    if (memorySlot == NULL) HICR_THROW_LOGIC("The memory slot is not supported by this backend\n");

    lpf_deregister(_lpf, memorySlot->getLPFSlot());
  }

  /**
   * This function pulls the received message count via LPF
   * from an LPF IB Verbs slot and sets the messagesRecv field of
   * a HiCR memory slot
   * @param[inout] memorySlot whose messageRecv should be updated
   */
  __USED__ inline void pullMessagesRecv(lpf::L0::GlobalMemorySlot* memorySlot)
  {
    size_t msg_cnt;
    lpf_memslot_t lpfSlot = memorySlot->getLPFSlot();
    lpf_get_rcvd_msg_count_per_slot(_lpf, &msg_cnt, lpfSlot);
    for (size_t i = initMsgCnt[memorySlot] + memorySlot->getMessagesRecv(); i < msg_cnt; i++)
      memorySlot->increaseMessagesRecv();
  }

  __USED__ inline void flush() override
  {
    lpf_flush(_lpf);
  }

  __USED__ inline bool acquireGlobalLockImpl(std::shared_ptr<HiCR::L0::GlobalMemorySlot> memorySlot) override
  {
    HICR_THROW_RUNTIME("Not yet implemented for this backend");
  }

  __USED__ inline void releaseGlobalLockImpl(std::shared_ptr<HiCR::L0::GlobalMemorySlot> memorySlot) override
  {
    HICR_THROW_RUNTIME("Not yet implemented for this backend");
  }
};

} // namespace L1

} // namespace lpf

} // namespace backend

} // namespace HiCR
