/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file common.hpp
 * @brief This file implements common definitions required by TaskR
 * @author Sergio Martin
 * @date 8/8/2023
 */

#pragma once

#include <functional>
#include <taskr/extern/atomic_queue/atomic_queue.h>
#include <taskr/extern/parallel_hashmap/phmap.h>

namespace taskr
{

/**
 * Task label type
 */
typedef uint64_t taskLabel_t;

#ifndef MAX_SIMULTANEOUS_WORKERS
  /**
   * Maximum simultaneous workers supported
   */
  #define MAX_SIMULTANEOUS_WORKERS 1024
#endif

#ifndef MAX_SIMULTANEOUS_TASKS
  /**
   * Maximum simultaneous tasks supported
   */
  #define MAX_SIMULTANEOUS_TASKS 65536
#endif

/**
 * Templated Lockfree queue definition
 */
template <class T, unsigned int N>
using lockFreeQueue_t = atomic_queue::AtomicQueue<T, N, (T)NULL>;

/**
 * Task callback function definition
 */
typedef std::function<void()> callback_t;

/**
 * Configuration for parallel hash sets
 */
#define SETNAME phmap::parallel_flat_hash_set

/**
 * Configuration for parallel hash sets
 */
#define SETEXTRAARGS                                                    \
  , phmap::priv::hash_default_hash<V>, phmap::priv::hash_default_eq<V>, \
    std::allocator<V>, 4, std::mutex

/**
 * Template for parallel hash sets
 */
template <class V>
using HashSetT = SETNAME<V SETEXTRAARGS>;

} // namespace taskr
