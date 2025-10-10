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
 * @brief This file implements the shared memory mechanism to exchange slots for the pthreads backend
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
 * Implementation of the Pthreads shared memory space to exchange global memory slots among HiCR instances.
 * It holds a shared space among threads involved in the communication where one can exchange, retrieve, and destroy global memory slots
 * It can be created only by \ref SharedMemoryFactory
 *
 * This backend uses pthread-based mutexes and barriers to prevent concurrent access violations
 */
class SharedMemory
{
  /**
   * The factory is a friend class that can call the constructor
  */
  friend class SharedMemoryFactory;

  /**
 * Identifier for shared memory
*/
  using sharedMemoryId_t = uint64_t;

  public:

  ~SharedMemory()
  {
    // Destroy barrier
    pthread_barrier_destroy(&_barrier);

    // Destroy mutex
    pthread_mutex_destroy(&_mutex);
  }

  // Disable object copy
  SharedMemory(const SharedMemory &)            = delete;
  SharedMemory &operator=(const SharedMemory &) = delete;
  SharedMemory(SharedMemory &&)                 = delete;
  SharedMemory &operator=(SharedMemory &&)      = delete;

  /**
   * Add an element into the shared memory space
   * 
   * \param[in] tag slot tag
   * \param[in] key slot key
   * \param[in] slot global memory slot
  */
  __INLINE__ void insert(const GlobalMemorySlot::tag_t tag, const GlobalMemorySlot::globalKey_t key, const std::shared_ptr<GlobalMemorySlot> &slot)
  {
    // Lock the resource
    pthread_mutex_lock(&_mutex);

    // Add the memory slot
    _globalMemorySlots[tag][key] = slot;

    // Unlock the resource
    pthread_mutex_unlock(&_mutex);
  }

  /**
   * Retrieve a global memory slot
   * 
   * \param[in] tag slot tag
   * \param[in] key slot key
   * 
   * \return shared pointer to global memory slot if present, nullptr otherwise
  */
  __INLINE__ std::shared_ptr<HiCR::GlobalMemorySlot> get(const GlobalMemorySlot::tag_t tag, const GlobalMemorySlot::globalKey_t key) const
  {
    std::shared_ptr<HiCR::GlobalMemorySlot> value = nullptr;

    // Lock the resource
    pthread_mutex_lock(&_mutex);

    // Look for the tag
    if (_globalMemorySlots.find(tag) != _globalMemorySlots.end())
    {
      // Look for the key
      if (_globalMemorySlots.at(tag).find(key) != _globalMemorySlots.at(tag).end()) { value = _globalMemorySlots.at(tag).at(key); }
    }

    // Unlock the resource
    pthread_mutex_unlock(&_mutex);

    return value;
  }

  /**
   * Removes a global memory slot from the shared memory if present
   * 
   * \param[in] tag slot tag
   * \param[in] key slot key
  */
  __INLINE__ void remove(const GlobalMemorySlot::tag_t tag, const GlobalMemorySlot::globalKey_t key)
  {
    // Lock the resource
    pthread_mutex_lock(&_mutex);

    // Look for the tag
    if (_globalMemorySlots.find(tag) != _globalMemorySlots.end())
    {
      // Look for the key
      if (_globalMemorySlots[tag].find(key) != _globalMemorySlots[tag].end()) { _globalMemorySlots[tag].erase(key); }
    }

    // Unlock the resource
    pthread_mutex_unlock(&_mutex);
  }

  /**
 * Return the pair key-slots for a given tag
 * 
 * \param[in] tag
 * 
 * \return key-slots pair, empty map otherwise
*/
  __INLINE__ CommunicationManager::globalKeyToMemorySlotMap_t getKeyMemorySlots(const GlobalMemorySlot::tag_t tag) const
  {
    CommunicationManager::globalKeyToMemorySlotMap_t keyMemorySlots;

    // Lock the resource
    pthread_mutex_lock(&_mutex);

    // Search for the tag
    auto it = _globalMemorySlots.find(tag);

    // Fail if not found
    if (it == _globalMemorySlots.end())
    {
      // Unlock the resource
      pthread_mutex_unlock(&_mutex);

      return keyMemorySlots;
    }

    // Unlock the resource
    pthread_mutex_unlock(&_mutex);

    // Return key slot pairs
    return it->second;
  }

  /**
   * A barrier implementation that synchronizes all threads in the HiCR instance
   */
  __INLINE__ void barrier() { pthread_barrier_wait(&_barrier); }

  /**
   * Id getter
   * 
   * \return Identifier of the shared memory instance
   */
  __INLINE__ sharedMemoryId_t getId() const { return _id; }

  private:

  /**
   * Private constructor. Can be called only by \ref SharedMemoryFactory
   * 
   * \param[in] id Identifier for the instance of shared memory
   * \param[in] fenceCount barrier size. Indicates how many threads should reach the barrier before continuing
  */
  SharedMemory(const sharedMemoryId_t id, const size_t fenceCount)
    : _id(id),
      _fenceCount(fenceCount)
  {
    // Init barrier
    pthread_barrier_init(&_barrier, nullptr, _fenceCount);

    // Init mutex
    pthread_mutex_init(&_mutex, nullptr);
  }

  /**
   * Shared Memory ID
  */
  const sharedMemoryId_t _id;

  /**
   * Stores a barrier object to check on a barrier operation
   */
  pthread_barrier_t _barrier{};

  /**
   * Mutex to enable thread safety in the class.
   * Mutability allows const getter functions to lock the mutex, because this does not modify the logical
   * state of the shared memory
   */
  mutable pthread_mutex_t _mutex{};

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
