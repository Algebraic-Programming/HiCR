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

#include <hicr/exceptions.hpp>

namespace HiCR
{

namespace channel
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
  CircularBuffer(size_t capacity, __volatile__ size_t *headAdvanceCounter, __volatile__ size_t *tailAdvanceCounter) : _capacity(capacity),
                                                                                                                      _headAdvanceCounter(headAdvanceCounter),
                                                                                                                      _tailAdvanceCounter(tailAdvanceCounter)
  {
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
   * This function when called on a valid circular buffer instance will never fail.
   */
  __USED__ inline size_t getHeadPosition() const noexcept
  {
    return *_headAdvanceCounter % _capacity;
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
   * This function when called on a valid circular buffer instance will never fail.
   */
  __USED__ inline size_t getTailPosition() const noexcept
  {
    return *_tailAdvanceCounter % _capacity;
  }

  /**
   * This function increases the circular buffer depth (e.g., when an element is pushed) by advancing a virtual head.
   * The head cannot advance in such a way that the depth exceeds capacity.
   *
   * \param[in] n The number of positions to advance
   */
  __USED__ inline void advanceHead(const size_t n = 1)
  {
    // Current depth
    const auto curDepth = getDepth();

    // Calculating new depth
    const auto newDepth = curDepth + n;

    // Sanity check
    if (newDepth > _capacity) HICR_THROW_FATAL("New buffer depth (_depth (%lu) + n (%lu) = %lu) exceeded capacity (%lu) on increase. This is probably a bug in HiCR.\n", curDepth, n, newDepth, _capacity);

    // Advance head
    *_headAdvanceCounter = *_headAdvanceCounter + n;
  }

  /**
   * This function advances buffer tail (e.g., when an element is popped). It goes back around if the capacity is exceeded
   * The tail cannot advanced more than the current depth (that would mean that more elements were consumed than pushed).
   *
   * \param[in] n The number of positions to advance the head of the circular buffer
   */
  __USED__ inline void advanceTail(const size_t n = 1)
  {
    // Current depth
    const auto curDepth = getDepth();

    // Sanity check
    if (n > curDepth) HICR_THROW_FATAL("Circular buffer depth (%lu) smaller than number of elements to decrease on advance tail. This is probably a bug in HiCR.\n", curDepth, n);

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
  __USED__ inline size_t getCapacity() const noexcept
  {
    return _capacity;
  }

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
  __USED__ inline size_t getDepth() const noexcept
  {
    return calculateDepth(*_headAdvanceCounter, *_tailAdvanceCounter);
  }

  /**
   * This function can be used to quickly check whether the circular buffer is full.
   *
   * It affects the internal state of the circular buffer because it detects any updates in the internal state of the buffers
   *
   * \returns true, if the buffer is full
   * \returns false, if the buffer is not full
   */
  __USED__ inline bool isFull() const noexcept
  {
    return getDepth() == _capacity;
  }

  /**
   * This function can be used to quickly check whether the circular buffer is empty.
   *
   * It does not affects the internal state of the circular buffer
   *
   * \returns true, if the buffer is empty
   * \returns false, if the buffer is not empty
   */
  __USED__ inline bool isEmpty() const noexcept
  {
    return *_headAdvanceCounter == *_tailAdvanceCounter;
  }

  /**
   * Forces the head advance counter into a specific absolute value
   *
   * @param[in] headAdvanceCounter the new value of the head advance counter. This value should never be smaller than the current tail advance counter, othewise this means the circular buffer has negative depth
   */
  __USED__ inline void setHead(const size_t headAdvanceCounter)
  {
    // Sanity check
    if (*_tailAdvanceCounter > headAdvanceCounter) HICR_THROW_FATAL("Circular buffer new head advance value is smaller than tail's (%lu < %lu). This is probably a bug in HiCR.\n", headAdvanceCounter, *_tailAdvanceCounter);

    // Calculating new depth
    const auto newDepth = calculateDepth(headAdvanceCounter, *_tailAdvanceCounter);

    // Sanity check
    if (newDepth > _capacity) HICR_THROW_FATAL("Circular new buffer depth (%lu) exceeded capacity (%lu) on setHead. This is probably a bug in HiCR.\n", newDepth, _capacity);

    // Setting head
    *_headAdvanceCounter = headAdvanceCounter;
  }

  /**
   * Forces the tail advance counter into a specific absolute value
   *
   * @param[in] tailAdvanceCounter the new value of the head advance counter. This value should never be smaller than the current tail advance counter, othewise this means the circular buffer has negative depth
   */
  __USED__ inline void setTail(const size_t tailAdvanceCounter)
  {
    // Sanity check
    if (tailAdvanceCounter > *_headAdvanceCounter) HICR_THROW_FATAL("Circular buffer new tail advance value exceeds head (%lu > %lu). This is probably a bug in HiCR.\n", tailAdvanceCounter, *_headAdvanceCounter);

    // Calculating new depth
    const auto newDepth = calculateDepth(*_headAdvanceCounter, tailAdvanceCounter);

    // Sanity check
    if (newDepth > _capacity) HICR_THROW_FATAL("Circular buffer new buffer depth (%lu) exceeded capacity (%lu) on setTail. This is probably a bug in HiCR.\n", newDepth, _capacity);

    // Setting head
    *_tailAdvanceCounter = tailAdvanceCounter;
  }

  /**
   * @returns The absolute counter for the number of times the head was advanced
   */
  __USED__ inline size_t getHeadAdvanceCounter() const noexcept
  {
    return *_headAdvanceCounter;
  }

  /**
   * @returns The absolute counter for the number of times the tail was advanced
   */
  __USED__ inline size_t getTailAdvanceCounter() const noexcept
  {
    return *_tailAdvanceCounter;
  }

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

  private:

  __USED__ static inline size_t calculateDepth(const size_t headAdvanceCounter, const size_t tailAdvanceCounter)
  {
    return headAdvanceCounter - tailAdvanceCounter;
  }
};

} // namespace channel

} // namespace HiCR
