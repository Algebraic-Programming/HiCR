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
 * @file instancePool.hpp
 * @brief This file implements the pool that holds currently alive pthreads instances
 * @author L. Terracciano
 * @date 30/9/2025
 */

#pragma once

#include <atomic>
#include <pthread.h>
#include <cstdint>

#include <hicr/core/definitions.hpp>
#include <hicr/core/exceptions.hpp>
#include <hicr/core/instanceManager.hpp>

namespace HiCR::backend::pthreads
{

/**
 * Implementation of the Pthreads instance pool space to exchange global memory slots among HiCR instances.
 * It holds a shared space among threads involved in the communication where one can exchange, retrieve, and destroy global memory slots
 * It can be created only by \ref InstancePoolFactory
 *
 * This backend uses pthread-based mutexes and barriers to prevent concurrent access violations
 */
class InstancePool
{
  // /**
  //  * The factory is a friend class that can call the constructor
  // */
  // friend class InstancePoolFactory;

  /**
 * Identifier for instance pool
*/
  using instancePoolId_t = uint64_t;

  public:

  /**
   * Private constructor. Can be called only by \ref InstancePoolFactory
   * 
   * \param[in] id Identifier for the instance of instance pool
  */
  InstancePool(const instancePoolId_t id)
    : _id(id)
  {
    // Init mutex
    pthread_mutex_init(&_mutex, nullptr);

    // Set barrier count to 1. There is at least one instance in the system
    _barrierCount = 1;

    // Init barrier with new barrier count
    pthread_barrier_init(&_barrier, nullptr, _barrierCount);
  }

  ~InstancePool()
  {
    // Destroy mutex
    pthread_mutex_destroy(&_mutex);

    // Destroy barrier
    pthread_barrier_destroy(&_barrier);
  }

  // Disable object copy
  InstancePool(const InstancePool &)            = delete;
  InstancePool &operator=(const InstancePool &) = delete;
  InstancePool(InstancePool &&)                 = delete;
  InstancePool &operator=(InstancePool &&)      = delete;

  /**
   * Add a new instance to the pool
   * 
   * \param[in] instance instance to add
   * 
   * \note this call is not thread-safe
  */
  __INLINE__ void insertInstance(std::shared_ptr<HiCR::Instance> instance) { _instances.push_back(instance); }

  /**
   * Remove an instance from the pool
   * 
   * \param[in] instance instance to remove
   * 
   * \note this call is not thread-safe
  */
  __INLINE__ void deleteInstance(std::shared_ptr<HiCR::Instance> instance)
  {
    _instances.erase(std::remove_if(_instances.begin(), _instances.end(), [&](std::shared_ptr<HiCR::Instance> &i) { return i->getId() == instance->getId(); }), _instances.end());
  }

  /**
   * Get instances in the pool
   *
   * \return a copy of the instances vector 
   * 
   * \note this call is not thread-safe
  */
  __INLINE__ const InstanceManager::instanceList_t getInstances() const { return _instances; }

  /**
   * Update the barrier with a new count. It destroys and creates a new barrier, only
   * if \ref barrierCount is different than the current one. Otherwise the current barrier is kept.
   * 
   * \param[in] barrierCount the new count to be used for the barrier
   * 
   * \note this call is not thread-safe
  */
  __INLINE__ void updateBarrier(size_t barrierCount)
  {
    // Fail if the barrier count does not involve any thread
    if (barrierCount < 0) { HICR_THROW_RUNTIME("Can not have a barrier with barrier count %lu", barrierCount); }

    // Do nothing if the new barrier count coincides with the current one
    if (_barrierCount == barrierCount) { return; }

    // Update barrier count
    _barrierCount = barrierCount;

    // Destroy old barrier
    pthread_barrier_destroy(&_barrier);

    // Init barrier with new barrierCount
    pthread_barrier_init(&_barrier, nullptr, _barrierCount);
  }

  /**
   * Lock the instance pool
  */
  __INLINE__ void lock() { pthread_mutex_lock(&_mutex); }

  /**
   * Unlock the instance pool
  */
  __INLINE__ void unlock() { pthread_mutex_unlock(&_mutex); }

  /**
   * A barrier that synchronizes all threads in the HiCR instance
   */
  __INLINE__ void barrier() { pthread_barrier_wait(&_barrier); }

  /**
   * Id getter
   * 
   * \return Identifier of the instance pool instance
   */
  __INLINE__ instancePoolId_t getId() const { return _id; }

  private:

  /**
   * Instance Pool ID
  */
  const instancePoolId_t _id;

  /**
   * Barrier to synchronize all the HiCR instances in the pool
   */
  pthread_barrier_t _barrier{};

  /**
   * Mutex to enable thread safety in the class.
   * Mutability allows const getter functions to lock the mutex, because this does not modify the logical
   * state of the instance pool
   */
  mutable pthread_mutex_t _mutex{};

  /**
   * Barrier count
   */
  size_t _barrierCount;

  /**
   * List of instances in the pool
  */
  InstanceManager::instanceList_t _instances;
};
} // namespace HiCR::backend::pthreads
