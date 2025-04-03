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
#include <lpf/noc.h>
#include <hicr/core/L0/localMemorySlot.hpp>
#include <hicr/core/L1/communicationManager.hpp>
#include "../L0/globalMemorySlot.hpp"
#include "../L0/localMemorySlot.hpp"

namespace HiCR::backend::lpf::L1
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
      _rank((lpf_pid_t)rank),
      _lpf(lpf),
      _localSwapSlot(LPF_INVALID_MEMSLOT)
  {}

  /**
   * Serializes a global memory slot, registered on the current instance, into a buffer that can be sent over the network
   * to other instances, without the need to engage in a collective operation.
   * LPF is responsible for the serialization of the slot, which contains registration information etc.
   *
   * @param[in] globalSlot The global memory slot to serialize
   * @return A pointer to the serialized representation of the global memory slot in a newly allocated buffer.
   * \note The user is responsible for freeing the buffer, using delete[]
   */
  __INLINE__ uint8_t *serializeGlobalMemorySlot(const std::shared_ptr<HiCR::L0::GlobalMemorySlot> &globalSlot) const override
  {
    char  *serialized;
    size_t size    = 0;
    auto   lpfSlot = dynamic_pointer_cast<lpf::L0::GlobalMemorySlot>(globalSlot);
    if (lpfSlot == nullptr) HICR_THROW_LOGIC("The memory slot is not compatible with this backend\n");

    lpf_memslot_t lpfMemSlot = lpfSlot->getLPFSlot();

    // This produces an allocated buffer that needs to be freed
    CHECK(lpf_noc_serialize_slot(_lpf, lpfMemSlot, &serialized, &size));

    uint8_t *ret = new uint8_t[sizeof(lpf_pid_t) + size + sizeof(size_t)];
    std::memcpy(ret, &_rank, sizeof(lpf_pid_t));
    std::memcpy(ret + sizeof(lpf_pid_t), &size, sizeof(size_t));
    std::memcpy(ret + sizeof(lpf_pid_t) + sizeof(size_t), serialized, size);

    // TODO: this is a temporary fix, the LPF API should be fixed to free the buffer
    free(serialized);

    return ret;
  }

  /**
   * Deserializes a global memory slot from a buffer.
   *
   * \note The returned slot will not have a swap slot associated with it.
   *
   * @param[in] buffer The buffer to deserialize the global memory slot from
   * @param[in] tag The tag to associate with the deserialized global memory slot
   * @return A pointer to the deserialized global memory slot
   */
  __INLINE__ std::shared_ptr<HiCR::L0::GlobalMemorySlot> deserializeGlobalMemorySlot(uint8_t *buffer, L0::GlobalMemorySlot::tag_t tag) override
  {
    // first <sizeof(lpf_pid_t)> bytes are the rank, followed by the size of the lpf slot buffer, the rest is the lpf slot buffer
    size_t        size;
    lpf_pid_t     rank;
    lpf_memslot_t slot = LPF_INVALID_MEMSLOT;

    std::memcpy(&rank, buffer, sizeof(lpf_pid_t));
    std::memcpy(&size, buffer + sizeof(lpf_pid_t), sizeof(size_t));
    uint8_t *serialized = buffer + sizeof(lpf_pid_t) + sizeof(size_t);

    CHECK(lpf_noc_register(_lpf, nullptr, 0, &slot));

    CHECK(lpf_noc_deserialize_slot(_lpf, (char *)serialized, slot));

    return std::make_shared<HiCR::backend::lpf::L0::GlobalMemorySlot>(rank, slot, LPF_INVALID_MEMSLOT, tag);
  }

  /**
   * Promotes a local memory slot to a global memory slot (see abstract class for more details)
   *
   * @param[in] memorySlot The local memory slot to promote
   * @param[in] tag The tag to associate with the promoted global memory slot
   * @return A pointer to the promoted global memory slot
   */
  __INLINE__ std::shared_ptr<HiCR::L0::GlobalMemorySlot> promoteLocalMemorySlot(const std::shared_ptr<HiCR::L0::LocalMemorySlot> &memorySlot,
                                                                                HiCR::L0::GlobalMemorySlot::tag_t                 tag) override
  {
    auto lpfSlot = dynamic_pointer_cast<lpf::L0::LocalMemorySlot>(memorySlot);
    if (lpfSlot == nullptr) HICR_THROW_LOGIC("The memory slot is not supported by this backend\n");

    lpf_memslot_t promotedSlot = LPF_INVALID_MEMSLOT;
    lpf_memslot_t lpfSwapSlot  = LPF_INVALID_MEMSLOT;

    void  *ptr            = lpfSlot->getPointer();
    size_t size           = lpfSlot->getSize();
    void  *lpfSwapSlotPtr = lpfSlot->getLPFSwapPointer();

    CHECK(lpf_noc_register(_lpf, ptr, size, &promotedSlot));
    CHECK(lpf_noc_register(_lpf, lpfSwapSlotPtr, sizeof(uint64_t), &lpfSwapSlot)); // TODO: check if sizeof(uint64_t) is correct

    return std::make_shared<HiCR::backend::lpf::L0::GlobalMemorySlot>(_rank, promotedSlot, lpfSwapSlot, tag, 0 /* global key */, memorySlot);
  }

  /**
   * Destroys a (locally promoted) global memory slot. This operation is local.
   *
   * If the memory slot has not been created through #promoteLocalMemorySlot, the behavior is undefined.
   *
   * @param[in] memorySlot Memory slot to destroy.
   */
  __INLINE__ void destroyPromotedGlobalMemorySlot(const std::shared_ptr<HiCR::L0::GlobalMemorySlot> &memorySlot) override
  {
    // Getting up-casted pointer for the global memory slot
    auto lpfSlot = dynamic_pointer_cast<lpf::L0::GlobalMemorySlot>(memorySlot);

    // Checking whether the memory slot passed is compatible with this backend
    if (lpfSlot == nullptr) HICR_THROW_LOGIC("The memory slot is not supported by this backend\n");

    CHECK(lpf_noc_deregister(_lpf, lpfSlot->getLPFSlot()));

    // If the swap slot is not LPF_INVALID_MEMSLOT, deregister it
    if (lpfSlot->getLPFSwapSlot() != LPF_INVALID_MEMSLOT) CHECK(lpf_noc_deregister(_lpf, lpfSlot->getLPFSwapSlot()));
  }

  private:

  const size_t    _size;
  const lpf_pid_t _rank;
  const lpf_t     _lpf;

  /**
   * localSwap is the varable holding the value we
   * compare against the global value at globalSwap
   */
  uint64_t _localSwap{0ULL};

  /**
   * localSwapSlot is the LPF slot representing localSwap var
   */
  lpf_memslot_t _localSwapSlot;

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

    lpf_coll_t    coll;
    auto          globalSlotCounts        = std::vector<size_t>(_size);
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

    auto globalSlotCountsInBytes = std::vector<size_t>(_size);
    for (size_t i = 0; i < _size; i++) globalSlotCountsInBytes[i] = globalSlotCounts[i] * sizeof(size_t);

    auto globalSlotPidCountsInBytes = std::vector<size_t>(_size);
    for (size_t i = 0; i < _size; i++) globalSlotPidCountsInBytes[i] = globalSlotCounts[i] * sizeof(lpf_pid_t);

    // globalSlotSizes will hold exactly the union of all slot sizes at
    // each process (zero or more) to become global.

    auto localSlotSizes      = std::vector<size_t>(localSlotCount);
    auto localSlotKeys       = std::vector<HiCR::L0::GlobalMemorySlot::globalKey_t>(localSlotCount);
    auto localSlotProcessId  = std::vector<lpf_pid_t>(localSlotCount);
    auto globalSlotSizes     = std::vector<size_t>(globalSlotCount);
    auto globalSwapSlotSizes = std::vector<size_t>(globalSlotCount);
    auto globalSlotKeys      = std::vector<HiCR::L0::GlobalMemorySlot::globalKey_t>(globalSlotCount);
    auto globalSlotProcessId = std::vector<lpf_pid_t>(globalSlotCount);

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
    CHECK(lpf_register_local(_lpf, localSlotProcessId.data(), localSlotCount * sizeof(lpf_pid_t), &slot_local_process_ids));
    CHECK(lpf_register_global(_lpf, globalSlotProcessId.data(), globalSlotCount * sizeof(lpf_pid_t), &slot_global_process_ids));
    CHECK(lpf_sync(_lpf, LPF_SYNC_DEFAULT));
    // start allgatherv for process IDs assigned to each global slot
    CHECK(lpf_allgatherv(coll, slot_local_process_ids, slot_global_process_ids, globalSlotPidCountsInBytes.data(), false /*exclude myself*/));
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
    size_t localDestroySlotsCount = getGlobalMemorySlotsToDestroyPerTag()[tag].size();

    // Allgather the global slot to destroy counts
    auto globalDestroySlotCounts = std::vector<size_t>(_size);
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

    // If there are no slots to destroy from any instance, return to avoid a second round of collectives
    if (globalDestroySlotTotalCount == 0) return;

    // We need to destroy both the slot and the swap slot
    localDestroySlotsCount *= 2lu;
    globalDestroySlotTotalCount *= 2lu;

    auto globalDestroySlotCountsInBytes = std::vector<size_t>(_size);
    for (size_t i = 0; i < _size; i++) globalDestroySlotCountsInBytes[i] = globalDestroySlotCounts[i] * 2 * sizeof(size_t);

    // Allgather the global slot keys: Here we use the actual slot_t as the key! (contrary to MPI)
    // We need both the slot and the swap slot IDs
    auto localDestroySlotIds  = std::vector<lpf_memslot_t>(localDestroySlotsCount);
    auto globalDestroySlotIds = std::vector<lpf_memslot_t>(globalDestroySlotTotalCount);

    // Filling in the local IDs storage
    size_t i = 0;
    for (const auto &slot : getGlobalMemorySlotsToDestroyPerTag()[tag])
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
    CHECK(lpf_allgatherv(coll, slot_local_ids, slot_global_ids, globalDestroySlotCountsInBytes.data(), false /* exclude myself */));
    CHECK(lpf_sync(_lpf, LPF_SYNC_DEFAULT));
    CHECK(lpf_collectives_destroy(coll));
    CHECK(lpf_deregister(_lpf, slot_local_ids));
    CHECK(lpf_deregister(_lpf, slot_global_ids));
    // End allgather global slot IDs block

    // Deduplicate the global slot keys
    auto globalDestroySlotIdsSet = std::set<lpf_memslot_t>(globalDestroySlotIds.begin(), globalDestroySlotIds.end());

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
    // Getting up-casted pointer for the global memory slot
    auto memorySlot = dynamic_pointer_cast<lpf::L0::GlobalMemorySlot>(memorySlotPtr);

    // Checking whether the memory slot passed is compatible with this backend
    if (memorySlot == nullptr) HICR_THROW_LOGIC("The memory slot is not supported by this backend\n");

    // Deregistering from LPF
    CHECK(lpf_deregister(_lpf, memorySlot->getLPFSlot()));
    CHECK(lpf_deregister(_lpf, memorySlot->getLPFSwapSlot()));
  }

  __INLINE__ void memcpyImpl(const std::shared_ptr<HiCR::L0::LocalMemorySlot>  &destination,
                             size_t                                             dst_offset,
                             const std::shared_ptr<HiCR::L0::GlobalMemorySlot> &source,
                             size_t                                             src_offset,
                             size_t                                             size) override
  {
    // Getting up-casted pointer
    auto src = dynamic_pointer_cast<lpf::L0::GlobalMemorySlot>(source);

    // Checking whether the execution unit passed is compatible with this backend
    if (source == nullptr) HICR_THROW_LOGIC("The passed source memory slot is not supported by this backend\n");

    // Getting up-casted pointer
    auto dest = dynamic_pointer_cast<lpf::L0::LocalMemorySlot>(destination);

    // Checking whether the execution unit passed is compatible with this backend
    if (dest == nullptr) HICR_THROW_LOGIC("The passed destination memory slot is not supported by this backend\n");

    // Getting internal lpf slots
    lpf_memslot_t srcSlot = src->getLPFSlot();
    lpf_memslot_t dstSlot = dest->getLPFSlot();

    // Getting remote rank
    auto remoteRank = src->getRank();

    // Perform the get operation
    lpf_get(_lpf, remoteRank, srcSlot, src_offset, dstSlot, dst_offset, size, LPF_MSG_DEFAULT);
  }

  __INLINE__ void memcpyImpl(const std::shared_ptr<HiCR::L0::GlobalMemorySlot> &destination,
                             size_t                                             dst_offset,
                             const std::shared_ptr<HiCR::L0::LocalMemorySlot>  &source,
                             size_t                                             src_offset,
                             size_t                                             size) override
  {
    // Getting up-casted pointer
    auto src = dynamic_pointer_cast<lpf::L0::LocalMemorySlot>(source);

    // Checking whether the execution unit passed is compatible with this backend
    if (src == nullptr) HICR_THROW_LOGIC("The passed source memory slot is not supported by this backend\n");

    // Getting up-casted pointer
    auto dest = dynamic_pointer_cast<lpf::L0::GlobalMemorySlot>(destination);

    // Checking whether the execution unit passed is compatible with this backend
    if (dest == nullptr) HICR_THROW_LOGIC("The passed destination memory slot is not supported by this backend\n");

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
    if (getGlobalMemorySlotTagKeyMap().contains(tag) == false)
      HICR_THROW_LOGIC("getGlobalMemorySlots: Requesting a global memory slot for a tag (%lu) that has not been registered.", tag);
    globalKeyToMemorySlotMap_t slotsForTag = getGlobalMemorySlotTagKeyMap().at(tag);
    return slotsForTag;
  }

  /**
   * Implementaion of fence on a specific local memory slot for the LPF backend
   * \param[in] slot local memory slot to fence on
   * \param[in] expectedSent message count to be blockingly sent out on this slot
   * \param[in] expectedRcvd message count to be blockingly received on this slot
   */
  __INLINE__ void fenceImpl(const std::shared_ptr<HiCR::L0::LocalMemorySlot> &slot, size_t expectedSent, size_t expectedRcvd) override
  {
    auto          memSlot = dynamic_pointer_cast<lpf::L0::LocalMemorySlot>(slot);
    lpf_memslot_t lpfSlot = memSlot->getLPFSlot();
    if (lpfSlot == LPF_INVALID_MEMSLOT) { HICR_THROW_LOGIC("This slot is not registered with LPF!"); }
    CHECK(lpf_counting_sync_per_slot(_lpf, LPF_SYNC_DEFAULT, lpfSlot, memSlot->getMessagesSent() + expectedSent, memSlot->getMessagesRecv() + expectedRcvd));

    setMessagesSent(*memSlot, memSlot->getMessagesSent() + expectedSent);
    setMessagesRecv(*memSlot, memSlot->getMessagesRecv() + expectedRcvd);
  }

  /**
   * This method passively updates the received message count
   * on a memory slot from LPF. It does not ask LPF to do any
   * additional polling on messages.
   * \param[out] memorySlot HiCr memory slot to update
   */
  __INLINE__ void updateMessagesRecv(const std::shared_ptr<HiCR::L0::LocalMemorySlot> &memorySlot)
  {
    size_t        msg_cnt = 0;
    auto          memSlot = dynamic_pointer_cast<lpf::L0::LocalMemorySlot>(memorySlot);
    lpf_memslot_t lpfSlot = memSlot->getLPFSlot();
    lpf_get_rcvd_msg_count_per_slot(_lpf, &msg_cnt, lpfSlot);
    setMessagesRecv(*memSlot, msg_cnt);
  }

  /**
   * This method passively updates the sent message count
   * on a memory slot from LPF. It does not ask LPF to do any
   * additional polling on messages.
   * \param[out] memorySlot HiCr memory slot to update
   */
  __INLINE__ void updateMessagesSent(const std::shared_ptr<HiCR::L0::LocalMemorySlot> &memorySlot)
  {
    size_t        msg_cnt = 0;
    auto          memSlot = dynamic_pointer_cast<lpf::L0::LocalMemorySlot>(memorySlot);
    lpf_memslot_t lpfSlot = memSlot->getLPFSlot();
    lpf_get_sent_msg_count_per_slot(_lpf, &msg_cnt, lpfSlot);
    setMessagesSent(*memSlot, msg_cnt);
  }

  __INLINE__ void queryMemorySlotUpdatesImpl(std::shared_ptr<HiCR::L0::LocalMemorySlot> memorySlot) override { fenceImplSingle(memorySlot); }

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

  private:

  /**
   * Implementaion of fence on a specific local memory slot for the LPF backend
   * \param[in] hicrSlot local memory slot to fence on
   */
  __INLINE__ void fenceImplSingle(const std::shared_ptr<HiCR::L0::LocalMemorySlot> &hicrSlot)
  {
    auto          memorySlot = dynamic_pointer_cast<lpf::L0::LocalMemorySlot>(hicrSlot);
    lpf_memslot_t slot       = memorySlot->getLPFSlot();
    CHECK(lpf_sync_per_slot(_lpf, LPF_SYNC_DEFAULT, slot));
    updateMessagesRecv(memorySlot);
    updateMessagesSent(memorySlot);
  }
};

} // namespace HiCR::backend::lpf::L1
