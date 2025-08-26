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
 * @file circularBuffer.hpp
 * @brief Provides generic logic for circular buffers.
 * @author S. M. Martin
 * @date 16/11/2023
 */

#pragma once

#include <hicr/core/exceptions.hpp>

namespace HiCR::channel
{

/**
 * @brief Generic class type for circular buffer
 *
 * Abstracts away the implementation of a circular buffer with two pointers
 *   - Head Advance Counter: How many positions has the head been advanced
 *   - Tail Advance Counter: How many positions has the tail been advanced
 * Storage for these pointers shall be provided by the caller as pointers and will be considered volatile
 * (this is useful for RDMA changes to the internal state of the circular buffer).
 *
 */
class CircularBuffer
{
  public:

  /**
   * Constructor for the circular buffer class
   *
   * @param[in] capacity The maximum capacity (elements) in the buffer
   * @param[in] headAdvanceCounter A pointer to storage for the head advance counter
   * @param[in] tailAdvanceCounter A pointer to storage fot the tail advance counter
   */
  CircularBuffer(size_t capacity, __volatile__ size_t *headAdvanceCounter, __volatile__ size_t *tailAdvanceCounter)
    : _capacity(capacity),
      _headAdvanceCounter(headAdvanceCounter),
      _tailAdvanceCounter(tailAdvanceCounter)

  {}

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
   * This function when called on a valid circular buffer instance will never fail.
   */
  [[nodiscard]] __INLINE__ size_t getHeadPosition() const noexcept { return *_headAdvanceCounter % _capacity; }

  /**
   * @returns The current position of the buffer head
   *
   * Returns the position of the buffer head to set as offset for sending/receiving operations.
   *
   * This is a one-sided blocking call that need not be made collectively.
   *
   * This is a getter function that should complete in \f$ \Theta(1) \f$ time.
   *
   * This function when called on a valid circular buffer instance will never fail.
   */
  [[nodiscard]] __INLINE__ size_t getTailPosition() const noexcept { return *_tailAdvanceCounter % _capacity; }

  /**
   * This function increases the circular buffer depth (e.g., when an element is pushed) by advancing a virtual head.
   * The head cannot advance in such a way that the depth exceeds capacity.
   *
   * \param[in] n The number of positions to advance
   */
  __INLINE__ void advanceHead(const size_t n = 1)
  {
    // Current depth
    const auto curDepth = getDepth();

    // Calculating new depth
    const auto newDepth = curDepth + n;

    // Sanity check
    if (newDepth > _capacity)
      HICR_THROW_FATAL("New buffer depth (_depth (%lu) + n (%lu) = %lu) exceeded capacity (%lu) on increase. This is probably a bug in HiCR.\n", curDepth, n, newDepth, _capacity);

    // Advance head
    *_headAdvanceCounter = *_headAdvanceCounter + n;
  }

  /**
   * This function advances buffer tail (e.g., when an element is popped). It goes back around if the capacity is exceeded
   * The tail cannot advanced more than the current depth (that would mean that more elements were consumed than pushed).
   *
   * \param[in] n The number of positions to advance the head of the circular buffer
   */
  __INLINE__ void advanceTail(const size_t n = 1)
  {
    // Current depth
    const auto curDepth = getDepth();

    // Sanity check
    if (n > curDepth)
      HICR_THROW_FATAL("Circular buffer depth (%lu) smaller than number of elements (%lu) to decrease on advance tail. This is probably a bug in HiCR.\n", curDepth, n);

    // Advance tail
    *_tailAdvanceCounter = *_tailAdvanceCounter + n;
  }

  /**
   * @returns The capacity of the circular buffer.
   *
   * This is a one-sided blocking call that need not be made collectively.
   *
   * This function when called on a valid circular buffer instance will never fail.
   */
  [[nodiscard]] __INLINE__ size_t getCapacity() const noexcept { return _capacity; }

  /**
   * Returns the current circular buffer depth.
   *
   * \note This is not a thread-safe call
   *
   * This is a getter function that should complete in \f$ \Theta(1) \f$ time.
   *
   * @returns The number of tokens in this circular buffer.
   *
   * This function when called on a valid circular buffer instance will never fail.
   */
  [[nodiscard]] __INLINE__ size_t getDepth() const noexcept { return calculateDepth(*_headAdvanceCounter, *_tailAdvanceCounter); }

  /**
   * This function can be used to quickly check whether the circular buffer is full.
   *
   * It affects the internal state of the circular buffer because it detects any updates in the internal state of the buffers
   *
   * \returns true, if the buffer is full
   * \returns false, if the buffer is not full
   */
  [[nodiscard]] __INLINE__ bool isFull() const noexcept { return getDepth() == _capacity; }

  /**
   * This function can be used to quickly check whether the circular buffer is empty.
   *
   * It does not affects the internal state of the circular buffer
   *
   * \returns true, if the buffer is empty
   * \returns false, if the buffer is not empty
   */
  [[nodiscard]] __INLINE__ bool isEmpty() const noexcept { return *_headAdvanceCounter == *_tailAdvanceCounter; }

  private:

  /**
   * How many tokens fit in the buffer
   */
  const size_t _capacity;

  /**
   * Stores how many position has the head advanced so far
   */
  __volatile__ size_t *const _headAdvanceCounter;

  /**
   * Stores how many position has the tail advanced so far
   */
  __volatile__ size_t *const _tailAdvanceCounter;

  protected:

  /**
   * Calculate depth of circular buffer
   * @param[in] headAdvanceCounter head index
   * @param[in] tailAdvanceCounter tail index
   * @return depth of buffer (in elements)
   */
  __INLINE__ static size_t calculateDepth(const size_t headAdvanceCounter, const size_t tailAdvanceCounter)
  {
    if (headAdvanceCounter < tailAdvanceCounter)
    {
      HICR_THROW_FATAL("Head index (%lu) < tail index (%lu). This is a critical bug in HiCR!\n", headAdvanceCounter, tailAdvanceCounter);
    }
    return headAdvanceCounter - tailAdvanceCounter;
  }
};

} // namespace HiCR::channel
