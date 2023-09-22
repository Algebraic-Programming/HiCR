
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

#include <hicr/backend.hpp>
#include <hicr/common/definitions.hpp>
#include <hicr/common/exceptions.hpp>

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
    return (_capacity + _head - _tail) % _capacity;
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
   * This function can be used to check the size of the token buffer that needs to be provided
   * to the channel.
   *
   * \return Size (bytes) of the token buffer
   */
  __USED__ static inline size_t getTokenBufferSize(const size_t tokenSize, const size_t capacity) noexcept
  {
    return tokenSize * capacity;
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

  /**
   * The constructor of the base Channel class.
   *
   * It requires the user to provide the allocated memory slots for the exchange (data) and coordination buffers.
   *
   * \internal For this implementation of channels to work correctly, the underlying backend should guarantee that
   * messages (one per token) should arrive in order. That is, if the producer sends tokens 'A' and 'B', the internal
   * counter for messages received for the data buffer should only increase after 'A' was received (even if B arrived
   * before. That is, if the received message counter starts as zero, it will transition to 1 and then to to 2, if
   * 'A' arrives before than 'B', or; directly to 2, if 'B' arrives before 'A'.
   *
   * \param[in] backend The backend that will facilitate communication between the producer and consumer sides
   * \param[in] tokenBuffer The memory slot pertaining to the data exchange buffer. The producer will push new
   * tokens into this buffer, while there is enough space. This buffer should be big enough to hold at least one
   * token.
   * \param[in] coordinationBuffer This is a small buffer to enable the consumer to signal how many tokens it has
   * popped. It may also be used for other coordination signals.
   * \param[in] tokenSize The size of each token.
   */
  Channel(Backend *backend,
    Backend::memorySlotId_t tokenBuffer,
    Backend::memorySlotId_t coordinationBuffer,
    const size_t tokenSize,
    const size_t capacity) :
  _backend(backend),
  _tokenBuffer(tokenBuffer),
  _coordinationBuffer(coordinationBuffer),
  // Registering a slot for the local variable specifiying the nuber of popped tokens, to transmit it to the producer
  _tailPositionSlot(backend->registerLocalMemorySlot(&_tail, sizeof(_tail))),
  _tokenSize(tokenSize),
  _capacity(capacity)
  {
    if (_tokenSize == 0) HICR_THROW_LOGIC("Attempting to create a channel with token size 0.\n");
    if (_capacity == 0) HICR_THROW_LOGIC("Attempting to create a channel with zero capacity \n");

    // Checking that the provided token exchange  buffer has the right size
    auto requiredTokenBufferSize = getTokenBufferSize(_tokenSize, _capacity);
    auto providedTokenBufferSize = _backend->getMemorySlotSize(_tokenBuffer);
    if (providedTokenBufferSize < requiredTokenBufferSize) HICR_THROW_LOGIC("Attempting to create a channel with a token data buffer size (%lu) smaller than the required size (%lu).\n", providedTokenBufferSize, requiredTokenBufferSize);

    // Checking that the provided coordination buffer has the right size
    auto requiredCoordinationBufferSize = getCoordinationBufferSize();
    auto providedCoordinationBufferSize = _backend->getMemorySlotSize(_coordinationBuffer);
    if (providedCoordinationBufferSize < requiredCoordinationBufferSize) HICR_THROW_LOGIC("Attempting to create a channel with a coordination buffer size (%lu) smaller than the required size (%lu).\n", providedCoordinationBufferSize, requiredCoordinationBufferSize);
  }

  ~Channel()
  {
   // Unregistering memory slot corresponding to popped token count
   _backend->deregisterLocalMemorySlot(_tailPositionSlot);
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
  Backend *const _backend;

  /**
   * Memory slot that represents the token buffer that producer sends data to
   */
  Backend::memorySlotId_t _tokenBuffer;

  /**
   * Memory slot that enables coordination communication from the consumer to the producer
   */
  Backend::memorySlotId_t _coordinationBuffer;

  /**
   * This function advances the circular buffer head (e.g., when an element is pushed). It goes back around if the capacity is exceeded
   * The head cannot advance in such a way that the depth exceeds capacity.
   *
   * \param[in] n The number of positions to advance the head of the circular buffer
   */
  __USED__ inline void advanceHead(const size_t n = 1)
  {
    // Calculating new depth
    size_t newDepth = getDepth() + n;

    // Sanity check
    if (newDepth > _capacity) HICR_THROW_FATAL("Channel's circular new buffer depth (_depth (%lu) + n (%lu) = %lu) exceeded capacity (%lu) on advance head. This is probably a bug in HiCR.\n", getDepth(), n, newDepth, _capacity);

    // Advance head
    _head += n;

    // Wrap around circular buffer, if necessary
    if (_head >= _capacity) _head = _head - _capacity;
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
    if (n > getDepth()) HICR_THROW_FATAL("Channel's circular buffer depth (%lu) smaller than number of elements to decrease on advance tail. This is probably a bug in HiCR.\n", getDepth(), n);

    // Advance head
    _tail += n;

    // Wrap around circular buffer, if necessary
    if (_tail >= _capacity) _tail = _tail - _capacity;
  }

  /**
   * Local memory slot to update the tail position
   */
  const Backend::memorySlotId_t _tailPositionSlot;

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
};

}; // namespace HiCR
