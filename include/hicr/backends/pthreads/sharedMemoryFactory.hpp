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
 * @file sharedMemoryFactory.hpp
 * @brief This file implements the factory to create shared memories to be used for the pthread communication manager
 * @author L. Terracciano
 * @date 30/9/2025
 */

#pragma once

#include <pthread.h>

#include "sharedMemory.hpp"

namespace HiCR::backend::pthreads
{

/**
 * Identifier for shared memory
*/
using sharedMemoryId_t = uint64_t;

/**
 * Shared memory factory class that creates and holds Shared Memory objects
 * 
 * This backend uses pthread-based mutexes and barriers to prevent concurrent access violations
 */
class SharedMemoryFactory
{
  public:

  /**
   * Constructor
  */
  SharedMemoryFactory() { pthread_mutex_init(&_mutex, nullptr); }

  /**
 * Destructor
*/
  ~SharedMemoryFactory() { pthread_mutex_destroy(&_mutex); }

  /**
   * Get a shared memory by its id. If not present, it will create one with the specifiec fence count
   * 
   * \param[in] id shared memory id
   * \param[in] fenceCount fence count
   * 
   * \return a shared memory reference
  */
  __INLINE__ SharedMemory &get(const sharedMemoryId_t id, const size_t fenceCount)
  {
    // Lock the resource
    pthread_mutex_lock(&_mutex);

    // Find the shared memory
    auto it = _sharedMemoryMap.find(id);

    // If there is none
    if (it == _sharedMemoryMap.end())
    {
      // Create a new shared memory
      auto  sharedMemoryPtr = std::unique_ptr<SharedMemory>(new SharedMemory(id, fenceCount));
      auto &sharedMemory    = *sharedMemoryPtr;

      // Store it into the global slots map
      _sharedMemoryMap.emplace(id, std::move(sharedMemoryPtr));

      // Unlock the resource
      pthread_mutex_unlock(&_mutex);

      // Return the newly created shared memory
      return sharedMemory;
    }

    // Unlock the resource
    pthread_mutex_unlock(&_mutex);

    // Return the already present shared memory
    return *it->second;
  }

  private:

  /**
   * A mutex to make sure threads do not bother each other during certain operations
   */
  pthread_mutex_t _mutex{};

  /**
   * Map of shared memory objects
   */
  std::unordered_map<sharedMemoryId_t, std::unique_ptr<SharedMemory>> _sharedMemoryMap;
};
} // namespace HiCR::backend::pthreads
