
/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file base.hpp
 * @brief Provides base functionality for a Multiple-Producer Single-Consumer Channel (MPSC) over HiCR
 * @author S. M Martin
 * @date 14/11/2023
 */

#pragma once

#include <hicr/L0/globalMemorySlot.hpp>
#include <hicr/L1/memoryManager.hpp>
#include <hicr/common/circularBuffer.hpp>
#include <hicr/common/definitions.hpp>
#include <hicr/common/exceptions.hpp>

/**
 * Establishes how many elements are required in the base coordination buffer
 */
#define _HICR_CHANNEL_COORDINATION_BUFFER_ELEMENT_COUNT 2

/**
 * Establishes how the type of elements required in the base coordination buffer
 */
#define _HICR_CHANNEL_COORDINATION_BUFFER_ELEMENT_TYPE size_t

/**
 * Establishes the value index for the head advance count
 */
#define _HICR_CHANNEL_HEAD_ADVANCE_COUNT_IDX 0

/**
 * Establishes the value index for the tail advance count
 */
#define _HICR_CHANNEL_TAIL_ADVANCE_COUNT_IDX 1

namespace HiCR
{

namespace L2
{

namespace channel
{

/**
 * Base class definition for a HiCR Multiple-Producer Single-Consumer Channel
 */
class Base : public common::CircularBuffer
{
  public:

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
    return _HICR_CHANNEL_COORDINATION_BUFFER_ELEMENT_COUNT * sizeof(_HICR_CHANNEL_COORDINATION_BUFFER_ELEMENT_TYPE);
  }

  /**
   * This function can be used to check the size of the coordination buffer that needs to be provided
   * in the creation of the producer channel
   *
   * \param[in] coordinationBuffer Memory slot corresponding to the coordination buffer
   */
  __USED__ static inline void initializeCoordinationBuffer(L0::LocalMemorySlot *coordinationBuffer)
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
  Base(L1::MemoryManager *memoryManager,
       L0::LocalMemorySlot  *const localCoordinationBuffer,
       const size_t tokenSize,
       const size_t capacity) : common::CircularBuffer(capacity,
                                                       ((_HICR_CHANNEL_COORDINATION_BUFFER_ELEMENT_TYPE *)localCoordinationBuffer->getPointer()) + _HICR_CHANNEL_HEAD_ADVANCE_COUNT_IDX,
                                                       ((_HICR_CHANNEL_COORDINATION_BUFFER_ELEMENT_TYPE *)localCoordinationBuffer->getPointer()) + _HICR_CHANNEL_TAIL_ADVANCE_COUNT_IDX),
                                _memoryManager(memoryManager),
                                _localCoordinationBuffer(localCoordinationBuffer),
                                _tokenSize(tokenSize)
  {
    if (_tokenSize == 0) HICR_THROW_LOGIC("Attempting to create a channel with token size 0.\n");
    if (_capacity == 0) HICR_THROW_LOGIC("Attempting to create a channel with zero capacity \n");

    // Checking that the provided coordination buffers have the right size
    auto requiredCoordinationBufferSize = getCoordinationBufferSize();
    auto providedCoordinationBufferSize = _localCoordinationBuffer->getSize();
    if (providedCoordinationBufferSize < requiredCoordinationBufferSize) HICR_THROW_LOGIC("Attempting to create a channel with a local coordination buffer size (%lu) smaller than the required size (%lu).\n", providedCoordinationBufferSize, requiredCoordinationBufferSize);
  }

  virtual ~Base() = default;

  protected:

  /**
   * Pointer to the backend that is in charge of executing the memory transfer operations
   */
  L1::MemoryManager *const _memoryManager;

  /**
   * Local storage of coordination metadata
   */
  L0::LocalMemorySlot *const _localCoordinationBuffer;

  /**
   * Token size
   */
  const size_t _tokenSize;
};

} // namespace channel

} // namespace L2

} // namespace HiCR
