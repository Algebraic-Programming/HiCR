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
 * @file core.hpp
 * @brief This file implements the core mechanism to exchange slots and detect instances for the pthreads backend
 * @author L. Terracciano
 * @date 30/9/2025
 */

#pragma once

#include <map>
#include <pthread.h>

#include <hicr/backends/pthreads/instance.hpp>
#include <hicr/core/communicationManager.hpp>
#include <hicr/core/instanceManager.hpp>

namespace HiCR::backend::pthreads
{

/**
 * Implementation of the Pthreads core to exchange global memory slots and detect HiCR instances.
 *
 * This backend uses pthread-based mutexes and barriers to prevent concurrent access violations
 */
class Core
{
  public:

  /**
   * Constructor
   * 
   * \param[in] instanceCount how many instances will be in the application
  */
  Core(const size_t instanceCount)
    : _fenceCount(instanceCount),
      _currentInstanceId(0)
  {
    // Init barrier
    pthread_barrier_init(&_barrier, nullptr, _fenceCount);

    // Init mutex
    pthread_mutex_init(&_globalSlotMutex, nullptr);
    pthread_mutex_init(&_instanceMutex, nullptr);
  }

  ~Core()
  {
    // Destroy barrier
    pthread_barrier_destroy(&_barrier);

    // Destroy mutex
    pthread_mutex_destroy(&_globalSlotMutex);
    pthread_mutex_destroy(&_instanceMutex);
  }

  /**
   * Add an element into the shared memory space
   * 
   * \param[in] tag slot tag
   * \param[in] key slot key
   * \param[in] slot global memory slot
   * 
   * \note this function is thread-safe
  */
  __INLINE__ void insertGlobalSlot(const GlobalMemorySlot::tag_t tag, const GlobalMemorySlot::globalKey_t key, const std::shared_ptr<GlobalMemorySlot> &slot)
  {
    // Lock the resource
    pthread_mutex_lock(&_globalSlotMutex);

    // Add the memory slot
    _globalMemorySlots[tag][key] = slot;

    // Unlock the resource
    pthread_mutex_unlock(&_globalSlotMutex);
  }

  /**
   * Retrieve a global memory slot
   * 
   * \param[in] tag slot tag
   * \param[in] key slot key
   * 
   * \return shared pointer to global memory slot if present, nullptr otherwise
   * 
   * \note this function is thread-safe
  */
  __INLINE__ std::shared_ptr<GlobalMemorySlot> getGlobalSlot(const GlobalMemorySlot::tag_t tag, const GlobalMemorySlot::globalKey_t key) const
  {
    std::shared_ptr<GlobalMemorySlot> value = nullptr;

    // Lock the resource
    pthread_mutex_lock(&_globalSlotMutex);

    // Look for the tag
    if (_globalMemorySlots.find(tag) != _globalMemorySlots.end())
    {
      // Look for the key
      if (_globalMemorySlots.at(tag).find(key) != _globalMemorySlots.at(tag).end()) { value = _globalMemorySlots.at(tag).at(key); }
    }

    // Unlock the resource
    pthread_mutex_unlock(&_globalSlotMutex);

    return value;
  }

  /**
   * Removes a global memory slot from the shared memory if present
   * 
   * \param[in] tag slot tag
   * \param[in] key slot key
   * 
   * \note this function is thread-safe
  */
  __INLINE__ void removeGlobalSlot(const GlobalMemorySlot::tag_t tag, const GlobalMemorySlot::globalKey_t key)
  {
    // Lock the resource
    pthread_mutex_lock(&_globalSlotMutex);

    // Look for the tag
    if (_globalMemorySlots.find(tag) != _globalMemorySlots.end())
    {
      // Look for the key
      if (_globalMemorySlots[tag].find(key) != _globalMemorySlots[tag].end()) { _globalMemorySlots[tag].erase(key); }
    }

    // Unlock the resource
    pthread_mutex_unlock(&_globalSlotMutex);
  }

  /**
   * Return the pair key-slots for a given tag
   * 
   * \param[in] tag
   * 
   * \return key-slots pair, empty map otherwise
   * 
   * \note this function is thread-safe
  */
  __INLINE__ CommunicationManager::globalKeyToMemorySlotMap_t getKeyMemorySlots(const GlobalMemorySlot::tag_t tag) const
  {
    // Lock the resource
    pthread_mutex_lock(&_globalSlotMutex);

    // Search for the tag
    auto it = _globalMemorySlots.find(tag);

    // Fail if not found
    if (it == _globalMemorySlots.end())
    {
      // Unlock the resource
      pthread_mutex_unlock(&_globalSlotMutex);

      // Return empty map
      return {};
    }

    // Unlock the resource
    pthread_mutex_unlock(&_globalSlotMutex);

    // Return key slot pairs
    return it->second;
  }

  /**
   * Synchronize all HiCR instances
   */
  __INLINE__ void fence() { pthread_barrier_wait(&_barrier); }

  /**
   * Get all the instances registered in the core
   * 
   * \return a list of hicr instances
   * 
   * \note this function is thread-safe
  */
  __INLINE__ InstanceManager::instanceList_t getInstances() const
  {
    // Storage for the instance list
    InstanceManager::instanceList_t instances;

    // Lock the instances
    pthread_mutex_lock(&_instanceMutex);

    // Populate list
    for (const auto &[_, i] : _pthreadsInstanceMap) { instances.push_back(i); }

    // Unlock the instances
    pthread_mutex_unlock(&_instanceMutex);

    return instances;
  }

  /**
   * Add a new instance in the core
   * 
   * \param[in] pthreadId pthread id of the new instance
   * 
   * \return the newly created instance 
   * 
   * \note this function is thread-safe
  */
  __INLINE__ std::shared_ptr<Instance> addInstance(const pthread_t pthreadId)
  {
    // Lock the instances
    pthread_mutex_lock(&_instanceMutex);

    // Create the new instance
    auto instance = std::make_shared<backend::pthreads::Instance>(_currentInstanceId++, pthreadId, 0);

    // Add it to the map
    _pthreadsInstanceMap[pthreadId] = instance;

    // Unlock the instances
    pthread_mutex_unlock(&_instanceMutex);

    return instance;
  }

  /**
   * Get a specific instance
   * 
   * \param[in] pthreadId pthread id of the instance to retrieve
   * 
   * \return the HiCR instance associated with the given pthread id
   * 
   * \note this function is thread-safe
  */
  __INLINE__ std::shared_ptr<Instance> getInstance(const pthread_t pthreadId) const
  {
    pthread_mutex_lock(&_instanceMutex);
    auto instance = _pthreadsInstanceMap.at(pthreadId);
    pthread_mutex_unlock(&_instanceMutex);
    return instance;
  }

  /**
   * Getter of the root instance id
   * 
   * \return the root instance id
  */
  __INLINE__ Instance::instanceId_t getRootInstanceId() const { return 0; }

  private:

  /**
   * Stores a barrier object to check on a barrier operation
   */
  pthread_barrier_t _barrier{};

  /**
   * Mutex to enable thread safety during global slots operations.
   * Mutability allows const getter functions to lock the mutex, because this does not modify the logical
   * state of the core
   */
  mutable pthread_mutex_t _globalSlotMutex{};

  /**
   * Mutex to enable thread safety during instances operations.
   * Mutability allows const getter functions to lock the mutex, because this does not modify the logical
   * state of the core
   */
  mutable pthread_mutex_t _instanceMutex{};

  /**
   * How many threads should reach the fence before proceeding
   */
  const size_t _fenceCount;

  /**
    * Map to track the exchanged slots among different threads
    */
  CommunicationManager::globalMemorySlotTagKeyMap_t _globalMemorySlots;

  /**
   * Current instance id to assign
  */
  Instance::instanceId_t _currentInstanceId;

  /**
   * Mapping of pthreads and hicr instances
  */
  std::map<pthread_t, std::shared_ptr<Instance>> _pthreadsInstanceMap;
};
} // namespace HiCR::backend::pthreads
