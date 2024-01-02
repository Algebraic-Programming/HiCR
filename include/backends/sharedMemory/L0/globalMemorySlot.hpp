/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file sharedMemory/L0/globalMemorySlot.hpp
 * @brief Provides a definition for the memory slot class for the shared memory backend
 * @author S. M. Martin
 * @date 19/10/2023
 */
#pragma once

#include <hicr/L0/localMemorySlot.hpp>
#include <hicr/L0/globalMemorySlot.hpp>
#include <pthread.h>

namespace HiCR
{

namespace backend
{

namespace sharedMemory
{

namespace L0
{

/**
 * This class represents an abstract definition for a global Memory Slot resource for the shared memory backend
 * 
 * It uses pthread mutex to enforce the mutual exclusion logic
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
    const HiCR::L0::GlobalMemorySlot::tag_t globalTag = 0,
    const HiCR::L0::GlobalMemorySlot::globalKey_t globalKey = 0,
    HiCR::L0::LocalMemorySlot* sourceLocalMemorySlot = NULL) : HiCR::L0::GlobalMemorySlot(globalTag, globalKey, sourceLocalMemorySlot)
  {
    pthread_mutex_init(&_mutex, NULL);
  }

  ~GlobalMemorySlot()
  {
    // Freeing mutex memory
    pthread_mutex_destroy(&_mutex);
  }

  /**
   * Attempts to lock memory lock using its pthread mutex object
   *
   * This function never blocks the caller
   *
   * @return True, if successful; false, otherwise.
   */
  __USED__ inline bool trylock() { return pthread_mutex_trylock(&_mutex) == 0; } 

  /**
   * Attempts to lock memory lock using its pthread mutex object
   *
   * This function might block the caller if the memory slot is already locked
   */
  __USED__ inline void lock() { pthread_mutex_lock(&_mutex); } 

  /**
   * Unlocks the memory slot, if previously locked by the caller
   */
  __USED__ inline void unlock() { pthread_mutex_unlock(&_mutex); } 

  private:

  /**
   * Internal memory slot mutex to enforce lock acquisition
   */
  pthread_mutex_t _mutex;
};

} // namespace L0

} // namespace sharedMemory

} // namespace backend

} // namespace HiCR
