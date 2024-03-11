/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file concurrentQueue.hpp
 * @brief Provides generic support for concurrent queues.
 * @author S. M. Martin
 * @date 14/8/2023
 */

#pragma once

#include <hicr/definitions.hpp>
#include <atomic_queue/atomic_queue.h>

namespace HiCR
{

namespace common
{

/**
 * Templated Lockfree queue definition
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
template <class P, size_t N>
class ConcurrentQueue
{
  public:

  /**
   * Function to push new objects in the queue. This is a thread-safe lock-free operation
   *
   * \param[in] obj The object to push into the queue.
   */
  __USED__ inline void push(P obj) { _queue.push(obj); }

  /**
   * Function to pop an object from the queue. Poping removes an object from the front of the queue and returns it to the caller. This is a thread-safe lock-free operation
   *
   * \return The until-now front object of the queue.
   */
  __USED__ inline P pop()
  {
    P obj = NULL;

    // Poping next object from the lock-free queue
    _queue.try_pop(obj);

    // If we did not find an object in the queue, then return a NULL pointer
    return obj;
  }

  /**
   * Function to determine whether the queue is currently empty or not
   *
   * \return True, if it is empty; false if its not
   */
  __USED__ inline bool isEmpty() { return _queue.was_empty(); }

  private:

  /**
   * Internal implementation of the concurrent queue
   */
  lockFreeQueue_t<P, N> _queue;
};

} // namespace common

} // namespace HiCR
