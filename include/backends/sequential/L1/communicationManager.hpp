/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file communicationManager.hpp
 * @brief This file implements the communication manager support for the sequential backend
 * @author S. M. Martin
 * @date 19/12/2023
 */

#pragma once

#include <atomic>
#include <cstring>
#include <unistd.h>
#include <hicr/definitions.hpp>
#include <hicr/exceptions.hpp>
#include <hicr/L1/communicationManager.hpp>

namespace HiCR
{

namespace backend
{

namespace sequential
{

namespace L1
{

/**
 * Implementation of the communication manager for the sequential backend.
 */
class CommunicationManager final : public HiCR::L1::CommunicationManager
{
  public:

  /**
   * The constructor is employed to create the barriers required to coordinate threads
   *
   * \param[in] fenceCount Specifies how many times a fence has to be called for it to release callers
   */
  CommunicationManager(const size_t fenceCount = 1) : HiCR::L1::CommunicationManager(), _fenceCount{fenceCount} {}
  ~CommunicationManager() = default;

  private:

  /**
   * Queries the backend to update the internal state of the memory slot.
   * One main use case of this function is to update the number of messages received and sent to/from this slot.
   * This is a non-blocking, non-collective function.
   *
   * \param[in] memorySlot Memory slot to query for updates.
   */
  __USED__ inline void queryMemorySlotUpdatesImpl(HiCR::L0::GlobalMemorySlot *memorySlot) override
  {
  }

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
    // Simply adding local memory slots to the global map
    for (const auto &entry : memorySlots)
    {
      // Getting global key
      auto globalKey = entry.first;

      // Getting local memory slot to promot
      auto memorySlot = entry.second;

      // Creating new memory slot
      auto globalMemorySlot = new HiCR::L0::GlobalMemorySlot(tag, globalKey, memorySlot);

      // Registering memory slot
      registerGlobalMemorySlot(globalMemorySlot);
    }
  }

  /**
   * Specifies how many times a fence has to be called for it to release callers
   */
  const size_t _fenceCount;

  /**
   * Common definition of a collection of memory slots
   */
  typedef std::map<HiCR::L0::GlobalMemorySlot::tag_t, size_t> fenceCountTagMap_t;

  /**
   * Counter for calls to fence, filtered per tag
   */
  fenceCountTagMap_t _fenceCountTagMap;

  /**
   * Implementation of the fence operation for the sequential memory backend. The assumption here is that no concurrency
   * is present, so we don't need any mutual exclusion mechanisms to increase the counters
   */
  __USED__ inline void fenceImpl(const HiCR::L0::GlobalMemorySlot::tag_t tag) override
  {
    // Increasing the counter for the fence corresponding to the tag
    _fenceCountTagMap[tag]++;

    // Until we reached the required count, wait on it
    while (_fenceCountTagMap[tag] % _fenceCount != 0)
      ;
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

  __USED__ inline bool acquireGlobalLockImpl(HiCR::L0::GlobalMemorySlot *memorySlot) override
  {
    // This function does not do anything because sequential applications
    // do not incur concurrency issues.

    return true;
  }

  __USED__ inline void releaseGlobalLockImpl(HiCR::L0::GlobalMemorySlot *memorySlot) override
  {
    // This function does not do anything because sequential applications
    // do not incur concurrency issues.
  }
};

} // namespace L1

} // namespace sequential

} // namespace backend

} // namespace HiCR
