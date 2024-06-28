/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file concurrent/queue.hpp
 * @brief Provides generic support for concurrent queues.
 * @author S. M. Martin
 * @date 14/8/2023
 */

#pragma once

#include <atomic_queue/atomic_queue.h>
#include <hicr/core/definitions.hpp>

namespace HiCR
{

namespace concurrent
{

/**
 * Templated Lockfree queue definition
 */
template <class T>
using lockFreeQueue_t = atomic_queue::AtomicQueueB<T>;

/**
 * @brief Generic class type for concurrent queues
 *
 * Abstracts away the implementation of a concurrent queue, providing thread-safety access.
 * It also attempts to provide low overhead accesses by avoiding the need of mutex mechanisms, in favor of atomics.
 *
 * @tparam T Represents a item type to be stored in the queue
 */
template <class P>
class Queue
{
  public:

  /**
   * Default constructor
   * 
   * \param[in] maxEntries Indicates the maximum amount of entries
  */
  Queue(const size_t maxEntries) { _queue = new lockFreeQueue_t<P *>(maxEntries); }

  ~Queue() { delete _queue; }

  /**
   * Function to push new objects in the queue. This is a thread-safe lock-free operation
   *
   * \param[in] obj The object to push into the queue.
   */
  __INLINE__ void push(P *obj) { _queue->push(obj); }

  /**
   * Function to pop an object from the queue. Poping removes an object from the front of the queue and returns it to the caller. This is a thread-safe lock-free operation
   *
   * \return The until-now front object of the queue.
   */
  __INLINE__ P *pop()
  {
    P *obj = NULL;

    // Poping next object from the lock-free queue
    _queue->try_pop(obj);

    // If we did not find an object in the queue, then return a NULL pointer
    return obj;
  }

  /**
   * Function to determine whether the queue is currently empty or not
   *
   * The past tense in "was" is deliverate, since it can be proven the return value had that
   * value but it cannot be guaranteed that it still has it
   * 
   * \return True, if it is empty; false if its not. Possibly changed now.
   */
  __INLINE__ bool wasEmpty() const { return _queue->was_empty(); }

  /**
   * Function to determine the current possible size of the queue
   *
   * The past tense in "was" is deliverate, since it can be proven the return value had that
   * value but it cannot be guaranteed that it still has it
   * 
   * \return The size of the queue, as last read by this thread. Possibly changed now.
   */
  __INLINE__ size_t wasSize() const { return _queue->was_size(); }

  private:

  /**
   * Internal implementation of the concurrent queue
   */
  lockFreeQueue_t<P *> *_queue;
};

} // namespace concurrent

} // namespace HiCR
