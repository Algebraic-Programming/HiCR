
/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file channel.hpp
 * @brief Provides functionality for a Channel over HiCR
 * @author A. N. Yzelman & S. M Martin
 * @date 28/7/2023
 */

#pragma once

#include <hicr/common/definitions.hpp>
#include <hicr/common/exceptions.hpp>
#include <hicr/backend.hpp>

namespace HiCR
{

/**
 * Class definition for a HiCR Channel
 *
 * It exposes the functionality to be expected for a channel
 *
 */
class Channel
{

public:

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
   * @returns The number of tokens in this channel.
   *
   * If the current channel is a consumer, it corresponds to how many tokens
   * may yet be consumed. If the current channel is a producer, it corresponds
   * the channel capacity minus the returned value equals how many tokens may
   * still be pushed.
   *
   * This is a one-sided blocking call that need not be made collectively.
   *
   * This is a getter function that should complete in \f$ \Theta(1) \f$ time.
   *
   * This function when called on a valid channel instance will never fail.
   */
  __USED__ inline size_t getDepth() const noexcept
  {
    return _depth;
  }

  /**
   * @returns The size of the tokens in this channel.
   *
   * Returns simply the size per token. All tokens have the same size.
   *
   * This is a one-sided blocking call that need not be made collectively.
   *
   * This is a getter function that should complete in \f$ \Theta(1) \f$ time.
   *
   * This function when called on a valid channel instance will never fail.
   */
  __USED__ inline size_t getTokenSize() const noexcept
  {
    return _tokenSize;
  }

  /**
   * This function can be used to check the size of the coordination buffer that needs to be provided
   * in the creation of the channel
   *
   * \return Size (bytes) of the coordination buffer
   */
  __USED__ static inline size_t getCoordinationBufferSize() noexcept
  {
    return sizeof(size_t);
  }

  protected:

  Channel(Backend* backend, Backend::memorySlotId_t exchangeBuffer, Backend::memorySlotId_t coordinationBuffer, const size_t tokenSize) :
   _backend(backend),
   _dataExchangeMemorySlot(exchangeBuffer),
   _coordinationMemorySlot(coordinationBuffer),
   _tokenSize(tokenSize),
   _capacity(tokenSize == 0 ? 0 : backend->getMemorySlotSize(exchangeBuffer) / tokenSize)
  {
   if (_tokenSize == 0) HICR_THROW_LOGIC("Attempting to create a channel with token size 0.\n");
   if (_capacity == 0) HICR_THROW_LOGIC("Attempting to create a channel with token size (%lu) larger than the buffer size (%lu).\n", tokenSize, backend->getMemorySlotSize(exchangeBuffer));
  }

  ~Channel()
  {
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
  __USED__ inline size_t getHeadPosition() const noexcept
  {
    return _head;
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
    return _tail;
  }

  protected:

  /**
   * Pointer to the backend that is in charge of executing the memory transfer operations
   */
  Backend* const _backend;

  /**
   * Memory slot that represents the target data buffer that producer sends data to
   */
  const Backend::memorySlotId_t _dataExchangeMemorySlot;

  /**
   * Memory slot that enables coordination communication from the consumer to the producer
   */
  const Backend::memorySlotId_t _coordinationMemorySlot;

  /**
   * This function advances the circular buffer head (e.g., when an element is pushed). It goes back around if the capacity is exceeded
   * The head cannot advance in such a way that the depth exceeds capacity.
   */
  __USED__ inline void advanceHead(const size_t n = 1) noexcept
  {
    // Calculating new depth
    size_t newDepth = _depth + n;

   // Sanity check
    if (newDepth > _capacity) HICR_THROW_FATAL("Channel's circular buffer depth (%lu) exceeded capacity (%lu) on advance head. This is probably a bug in HiCR.\n", _depth, _capacity);

    // Advance head
    _head += n;

    // Wrap around circular buffer, if necessary
    if (_head >= _capacity) _head = _head - _capacity;

    // Adjust depth
    _depth = newDepth;
  }

  /**
   * This function advances buffer tail (e.g., when an element is popped). It goes back around if the capacity is exceeded
   * The tail cannot advanced more than the current depth (that would mean that more elements were consumed than pushed).
   */
  __USED__ inline void advanceTail(const size_t n = 1) noexcept
  {
   // Sanity check
   if (n > _depth) HICR_THROW_FATAL("Channel's circular buffer depth (%lu) smaller than number of elements to decrease on advance tail. This is probably a bug in HiCR.\n", _depth, n);

    // Advance head
    _tail += n;

    // Wrap around circular buffer, if necessary
    if (_tail >= _capacity) _tail = _tail - _capacity;

    // Adjust depth
    _depth = _depth - n;
  }

  /**
   * Counts how many tokens have been pushed into the channel globally
   */
  size_t _pushedTokens = 0;

  /**
   * Counts how many tokens have been popped out of the channel globally
   */
  size_t _poppedTokens = 0;

  private:

  /**
   * Token size
   */
  const size_t _tokenSize;

  /**
   * How many tokens fit in the buffer
   */
  const size_t _capacity;

  /**
   * Buffer position at the head
   */
  size_t _head = 0;

  /**
   * Buffer position at the tail
   */
  size_t _tail = 0;

  /**
   * Current depth of the tokens saved in the buffer
   */
  size_t _depth = 0;
};

}; // namespace HiCR
