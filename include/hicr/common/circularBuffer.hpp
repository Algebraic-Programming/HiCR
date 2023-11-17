/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file circularBuffer.hpp
 * @brief Provides generic logic for circular buffers.
 * @author S. M. Martin
 * @date 16/11/2023
 */

#pragma once

namespace HiCR
{

namespace common
{

/**
 * @brief Generic class type for circular buffer
 *
 * Abstracts away the implementation of a circular buffer with two pointers
 *   - Head: It stores the position after the last added token
 *   - Tail: It stores the position after the last removed token
 *
 * This class does not contain the buffers, only the logic to access it based on previous head/tail advances
 *
 * The internal state of the buffer is calculated via the depth and tail values.
 * Storage for these pointers shall be provided by the caller as pointers and will be considered volatile
 * (useful for RDMA changes to the internal state of the circular buffer).
 *
 */
class CircularBuffer
{
 public:

 CircularBuffer(size_t capacity, __volatile__ size_t* depth, __volatile__ size_t* tail) : _capacity(capacity), _depth(depth), _tail(tail)
 {
  *_depth = 0;
  *_tail = 0;
 }

 virtual ~CircularBuffer() = default;


 /**
  * @returns The current position of the buffer head
  *
  * Returns the position of the buffer head to set as offset for sending/receiving operations.
  *
  * This is a one-sided blocking call that need not be made collectively.
  *
  * This is a getter function that should complete in \f$ \Theta(1) \f$ time.
  *
  * This function when called on a valid channel instance will never fail.
  */
 __USED__ inline size_t getHeadPosition() const noexcept
 {
   return (*_tail + *_depth) % _capacity;
 }

 /**
  * @returns The current position of the buffer head
  *
  * Returns the position of the buffer head to set as offset for sending/receiving operations.
  *
  * This is a one-sided blocking call that need not be made collectively.
  *
  * This is a getter function that should complete in \f$ \Theta(1) \f$ time.
  *
  * This function when called on a valid channel instance will never fail.
  */
 __USED__ inline size_t getTailPosition() const noexcept
 {
   return *_tail;
 }

 /**
  * This function increases the circular buffer depth (e.g., when an element is pushed) by advancing a virtual head.
  * The head cannot advance in such a way that the depth exceeds capacity.
  *
  * \param[in] n The number of positions to advance
  */
 __USED__ inline void advanceHead(const size_t n = 1)
 {
   // Calculating new depth
   const auto newDepth = *_depth + n;

   // Sanity check
   if (newDepth > _capacity) HICR_THROW_FATAL("Channel's circular new buffer depth (_depth (%lu) + n (%lu) = %lu) exceeded capacity (%lu) on increase. This is probably a bug in HiCR.\n", *_depth, n, newDepth, _capacity);

   // Storing new depth
   *_depth = newDepth;
 }

 /**
  * This function advances buffer tail (e.g., when an element is popped). It goes back around if the capacity is exceeded
  * The tail cannot advanced more than the current depth (that would mean that more elements were consumed than pushed).
  *
  * \param[in] n The number of positions to advance the head of the circular buffer
  */
 __USED__ inline void advanceTail(const size_t n = 1)
 {
   // Sanity check
   if (n > *_depth) HICR_THROW_FATAL("Channel's circular buffer depth (%lu) smaller than number of elements to decrease on advance tail. This is probably a bug in HiCR.\n", *_depth, n);

   // Decrease depth
   *_depth = *_depth - n;

   // Advance tail
   *_tail = (*_tail + n) % _capacity;
 }

 /**
   * @returns The capacity of the channel.
   *
   * This is a one-sided blocking call that need not be made collectively.
   *
   * This function when called on a valid channel instance will never fail.
   */
  __USED__ inline size_t getCapacity() const noexcept
  {
    return _capacity;
  }

  /**
   * Returns the current channel depth.
   *
   * If the current channel is a consumer, it corresponds to how many tokens
   * may yet be consumed. If the current channel is a producer, it corresponds
   * the channel capacity minus the returned value equals how many tokens may
   * still be pushed.
   *
   * \note This is not a thread-safe call
   *
   * This is a getter function that should complete in \f$ \Theta(1) \f$ time.
   *
   * @returns The number of tokens in this channel.
   *
   * This function when called on a valid channel instance will never fail.
   */
  __USED__ inline size_t getDepth() const noexcept
  {
    return *_depth;
  }

  /**
   * This function can be used to quickly check whether the channel is full.
   *
   * It affects the internal state of the channel because it detects any updates in the internal state of the buffers
   *
   * \returns true, if the buffer is full
   * \returns false, if the buffer is not full
   */
  __USED__ inline bool isFull() const noexcept
  {
    return *_depth == _capacity;
  }

  /**
   * This function can be used to quickly check whether the channel is empty.
   *
   * It does not affects the internal state of the channel
   *
   * \returns true, if the buffer is empty
   * \returns false, if the buffer is not empty
   */
  __USED__ inline bool isEmpty() const noexcept
  {
    return *_depth == 0;
  }

 protected:

 /**
  * How many tokens fit in the buffer
  */
 const size_t _capacity;

 /**
  * Buffer position at the head
  */
 __volatile__ size_t* const _depth;

 /**
  * Buffer position at the tail
  */
 __volatile__ size_t* const _tail;

};

} // namespace common

} // namespace HiCR
