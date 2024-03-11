/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file host/L0/globalMemorySlot.hpp
 * @brief Provides a definition for the global memory slot class for the host (CPU) memory backends
 * @author S. M. Martin
 * @date 19/10/2023
 */
#pragma once

#include <mutex>
#include <hicr/L0/localMemorySlot.hpp>
#include <hicr/L0/globalMemorySlot.hpp>

namespace HiCR
{

namespace backend
{

namespace host
{

namespace L0
{

/**
 * This class represents an abstract definition for a global Memory Slot resource for the host (CPU) backends
 *
 * It uses mutexes to enforce the mutual exclusion logic
 */
class GlobalMemorySlot final : public HiCR::L0::GlobalMemorySlot
{
  public:

  /**
   * Constructor for a MemorySlot class for the MPI backend
   *
   * \param[in] globalTag For global memory slots, indicates the subset of global memory slots this belongs to
   * \param[in] globalKey Unique identifier for that memory slot that this slot occupies.
   * \param[in] sourceLocalMemorySlot The local memory slot (if applicable) from whence this global memory slot is created
   */
  GlobalMemorySlot(
    const HiCR::L0::GlobalMemorySlot::tag_t       globalTag             = 0,
    const HiCR::L0::GlobalMemorySlot::globalKey_t globalKey             = 0,
    std::shared_ptr<HiCR::L0::LocalMemorySlot>    sourceLocalMemorySlot = NULL)
    : HiCR::L0::GlobalMemorySlot(globalTag, globalKey, sourceLocalMemorySlot)
  {}

  ~GlobalMemorySlot() = default;

  /**
   * Attempts to lock memory lock using its mutex object
   *
   * This function never blocks the caller
   *
   * @return True, if successful; false, otherwise.
   */
  __USED__ inline bool trylock() { return _mutex.try_lock(); }

  /**
   * Attempts to lock memory lock using its mutex object
   *
   * This function might block the caller if the memory slot is already locked
   */
  __USED__ inline void lock() { _mutex.lock(); }

  /**
   * Unlocks the memory slot, if previously locked by the caller
   */
  __USED__ inline void unlock() { _mutex.unlock(); }

  private:

  /**
   * Internal memory slot mutex to enforce lock acquisition
   */
  std::mutex _mutex;
};

} // namespace L0

} // namespace host

} // namespace backend

} // namespace HiCR
