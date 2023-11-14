
/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file base.hpp
 * @brief Provides base functionality for a Multiple-Producer Single-Consumer Channel over HiCR
 * @author S. M Martin
 * @date 14/11/2023
 */

#pragma once
#include <hicr/backends/memoryManager.hpp>
#include <hicr/common/definitions.hpp>
#include <hicr/common/exceptions.hpp>
#include <hicr/memorySlot.hpp>

namespace HiCR
{

namespace MPSC
{

/**
 * Base class definition for a HiCR Multiple-Producer Single-Consumer Channel
 */
class Base
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
   * in the creation of the producer channel
   *
   * \return Size (bytes) of the coordination buffer
   */
  __USED__ static inline size_t getCoordinationBufferSize() noexcept
  {
    return 2 * sizeof(size_t);
  }

  /**
   * This function can be used to check the size of the coordination buffer that needs to be provided
   * in the creation of the producer channel
   *
   * \param[in] coordinationBuffer Memory slot corresponding to the coordination buffer
   */
  __USED__ static inline void initializeCoordinationBuffer(MemorySlot *coordinationBuffer)
  {
    // Checking for correct size
    auto requiredSize = getCoordinationBufferSize();
    auto size = coordinationBuffer->getSize();
    if (size < requiredSize) HICR_THROW_LOGIC("Attempting to initialize coordination buffer size on a memory slot (%lu) smaller than the required size (%lu).\n", size, requiredSize);

    // Getting actual buffer of the coordination buffer
    auto bufferPtr = coordinationBuffer->getPointer();

    // Resetting all its values to zero
    memset(bufferPtr, 0, getCoordinationBufferSize());
  }

  /**
   * This function can be used to check the minimum size of the token buffer that needs to be provided
   * to the consumer channel.
   *
   * \param[in] tokenSize The expected size of the tokens to use in the channel
   * \param[in] capacity The expected capacity (in token count) to use in the channel
   * \return Minimum size (bytes) required of the token buffer
   */
  __USED__ static inline size_t getTokenBufferSize(const size_t tokenSize, const size_t capacity) noexcept
  {
    return tokenSize * capacity;
  }

  protected:

  /**
   * The constructor of the base Channel class.
   *
   * It requires the user to provide the allocated memory slots for the exchange (data) and coordination buffers.
   *
   * \param[in] memoryManager The backend's memory manager to facilitate communication between the producer and consumer sides
   * \param[in] tokenBuffer The memory slot pertaining to the data exchange buffer. This buffer needs to be allocated
   *            at the consumer side. The producer will push new tokens into this buffer, while there is enough space.
   *            This buffer should be big enough to hold the required capacity * tokenSize.
   * \param[in] coordinationBuffer This is a small buffer that needs to be allocated at the producer side.
   *            enables the consumer to signal how many tokens it has popped. It may also be used for other coordination signals.
   * \param[in] tokenSize The size of each token.
   * \param[in] capacity The maximum number of tokens that will be held by this channel
   *
   * \internal For this implementation of channels to work correctly, the underlying backend should guarantee that
   * messages (one per token) should arrive in order. That is, if the producer sends tokens 'A' and 'B', the internal
   * counter for messages received for the data buffer should only increase after 'A' was received (even if B arrived
   * before. That is, if the received message counter starts as zero, it will transition to 1 and then to to 2, if
   * 'A' arrives before than 'B', or; directly to 2, if 'B' arrives before 'A'.
   */
  Base(backend::MemoryManager *memoryManager,
          MemorySlot *const tokenBuffer,
          MemorySlot *const localCoordinationBuffer,
          MemorySlot *const globalCoordinationBuffer,
          const size_t tokenSize,
          const size_t capacity) : _memoryManager(memoryManager),
                                   _tokenBuffer(tokenBuffer),
                                   _localCoordinationBuffer(localCoordinationBuffer),
                                   _globalCoordinationBuffer(globalCoordinationBuffer),
                                   _tokenSize(tokenSize),
                                   _capacity(capacity),
                                   _depth((size_t*)localCoordinationBuffer->getPointer()),
                                   _tail(&_depth[1])
  {
    if (_tokenSize == 0) HICR_THROW_LOGIC("Attempting to create a channel with token size 0.\n");
    if (_capacity == 0) HICR_THROW_LOGIC("Attempting to create a channel with zero capacity \n");

    // Checking that the provided token exchange  buffer has the right size
    auto requiredTokenBufferSize = getTokenBufferSize(_tokenSize, _capacity);
    auto providedTokenBufferSize = _tokenBuffer->getSize();
    if (providedTokenBufferSize < requiredTokenBufferSize) HICR_THROW_LOGIC("Attempting to create a channel with a token data buffer size (%lu) smaller than the required size (%lu).\n", providedTokenBufferSize, requiredTokenBufferSize);

    // Checking that the provided coordination buffers have the right size
    auto requiredCoordinationBufferSize = getCoordinationBufferSize();
    auto providedLocalCoordinationBufferSize = _localCoordinationBuffer->getSize();
    if (providedLocalCoordinationBufferSize < requiredCoordinationBufferSize) HICR_THROW_LOGIC("Attempting to create a channel with a local coordination buffer size (%lu) smaller than the required size (%lu).\n", providedLocalCoordinationBufferSize, requiredCoordinationBufferSize);
    auto providedGlobalCoordinationBufferSize = _globalCoordinationBuffer->getSize();
    if (providedGlobalCoordinationBufferSize < requiredCoordinationBufferSize) HICR_THROW_LOGIC("Attempting to create a channel with a global coordination buffer size (%lu) smaller than the required size (%lu).\n", providedGlobalCoordinationBufferSize, requiredCoordinationBufferSize);
  }

  virtual ~Base() = default;

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


  protected:

  /**
   * Pointer to the backend that is in charge of executing the memory transfer operations
   */
  backend::MemoryManager *const _memoryManager;

  /**
   * Memory slot that represents the token buffer that producer sends data to
   */
  MemorySlot *const _tokenBuffer;

   /**
   * Memory slot that enables coordination communication from the consumer to the producer
   */
  MemorySlot *const _localCoordinationBuffer;

  /**
   * Memory slot that enables coordination communication from the consumer to the producer
   */
  MemorySlot *const _globalCoordinationBuffer;

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
  __volatile__ size_t* const _depth;

  /**
   * Buffer position at the tail
   */
  __volatile__ size_t* const _tail;
};

} // namespace MPSC

} // namespace HiCR
