/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file concurrentQueue.hpp
 * @desc Provides generic support for concurrent queues.
 * @author S. M. Martin
 * @date 14/8/2023
 */

#pragma once

#include <../../../extern/atomic_queue/atomic_queue.h>
#include <hicr/common/definitions.hpp>

namespace HiCR
{

class Task;

namespace common
{

/**
 * Internal datatype for lock-free queue entries
 */
template <class T, unsigned int N>
using lockFreeQueue_t = atomic_queue::AtomicQueue<T, N, (T)NULL>;

/**
 * @brief Generic class type for concurrent queues
 *
 * Abstracts away the implementation of a concurrent queue, providing thread-safety access.
 * It also attempts to provide low overhead accesses by avoiding the need of mutex mechanisms, in favor of atomics.
 *
 * @tparam P Represents a pointer to the item type to be stored in the queue
 * @tparam N Represents the maximum number of elements supported in the queue. For performance reasons, this class does not check whether this value is exceeded during runtime. If needed, this check must be implemented by the layer using this class.
 */
template <class P, unsigned int N>
class concurrentQueue
{
  private:
  // Internal implementation of the concurrent queue
  lockFreeQueue_t<P, N> _queue;

  public:
  inline void push(P obj)
  {
    _queue.push(obj);
  }

  inline P pop()
  {
    P obj = NULL;

    // Poping next object from the lock-free queue
    _queue.try_pop(obj);

    // If we did not find an object in the queue, then return a NULL pointer
    return obj;
  }
};

} // namespace common

} // namespace HiCR
