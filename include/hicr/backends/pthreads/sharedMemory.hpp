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
 * @file sharedMemory.hpp
 * @brief This file implements the shared memory mechanism to exchange slots for the Pthreads backend
 * @author L. Terracciano
 * @date 30/9/2025
 */

#pragma once

#include <pthread.h>

#include <hicr/core/communicationManager.hpp>

namespace HiCR::backend::pthreads
{

/**
 * Break circular dependency
 */
class SharedMemoryFactory;

/**
 * Implementation of the Pthreads shared memory space to exchange global memory slots among HiCR instances
 *
 * This backend uses pthread-based mutexes and barriers to prevent concurrent access violations
 */
class SharedMemory
{
  friend class SharedMemoryFactory;

  public:

  ~SharedMemory()
  {
    // Destroy barrier
    pthread_barrier_destroy(&_barrier);

    // Destroy mutex
    pthread_mutex_destroy(&_mutex);
  }

  SharedMemory(const SharedMemory &)            = delete;
  SharedMemory &operator=(const SharedMemory &) = delete;
  SharedMemory(SharedMemory &&)                 = delete;
  SharedMemory &operator=(SharedMemory &&)      = delete;

  __INLINE__ void insert(const GlobalMemorySlot::tag_t tag, const GlobalMemorySlot::globalKey_t key, const std::shared_ptr<GlobalMemorySlot> &globalMemorySlot)
  {
    // Lock the resource
    pthread_mutex_lock(&_mutex);

    // add the memory slot
    _globalMemorySlots[tag][key] = globalMemorySlot;

    // Unlock the resource
    pthread_mutex_unlock(&_mutex);
  }

  __INLINE__ std::shared_ptr<HiCR::GlobalMemorySlot> get(const GlobalMemorySlot::tag_t tag, const GlobalMemorySlot::globalKey_t globalKey)
  {
    std::shared_ptr<HiCR::GlobalMemorySlot> value = nullptr;

    pthread_mutex_lock(&_mutex);
    if (_globalMemorySlots.find(tag) != _globalMemorySlots.end())
    {
      if (_globalMemorySlots[tag].find(globalKey) != _globalMemorySlots[tag].end()) { value = _globalMemorySlots[tag][globalKey]; }
    }
    pthread_mutex_unlock(&_mutex);

    return value;
  }

  __INLINE__ void remove(const GlobalMemorySlot::tag_t tag, const GlobalMemorySlot::globalKey_t globalKey)
  {
    pthread_mutex_lock(&_mutex);
    if (_globalMemorySlots.find(tag) != _globalMemorySlots.end())
    {
      if (_globalMemorySlots[tag].find(globalKey) != _globalMemorySlots[tag].end()) { _globalMemorySlots[tag].erase(globalKey); }
    }
    pthread_mutex_unlock(&_mutex);
  }

  /**
   * A barrier implementation that synchronizes all threads in the HiCR instance
   */
  __INLINE__ void barrier() { pthread_barrier_wait(&_barrier); }

  private:

  SharedMemory(const size_t fenceCount)
    : _fenceCount(fenceCount)
  {
    // Init barrier
    pthread_barrier_init(&_barrier, nullptr, _fenceCount);

    // Init mutex
    pthread_mutex_init(&_mutex, nullptr);
  }

  /**
   * Stores a barrier object to check on a barrier operation
   */
  pthread_barrier_t _barrier{};

  /**
   * A mutex to make sure threads do not bother each other during certain operations
   */
  pthread_mutex_t _mutex{};

  /**
   * How many threads should reach the fence before proceeding
   */
  const size_t _fenceCount;

  /**
    * Map to track the exchanged slots among different threads
    */
  CommunicationManager::globalMemorySlotTagKeyMap_t _globalMemorySlots;
};
} // namespace HiCR::backend::pthreads
