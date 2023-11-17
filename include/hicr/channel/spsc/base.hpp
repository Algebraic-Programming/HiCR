
/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file base.hpp
 * @brief Provides the base functionality for a Single-Producer Single-Consumer (SPSC) Channel over HiCR
 * @author A. N. Yzelman & S. M Martin
 * @date 28/7/2023
 */

#pragma once

#include <hicr/common/definitions.hpp>
#include <hicr/common/exceptions.hpp>
#include <hicr/common/circularBuffer.hpp>
#include <hicr/backends/memoryManager.hpp>
#include <hicr/memorySlot.hpp>

namespace HiCR
{

namespace channel
{

namespace SPSC
{

/**
 * Class definition for a HiCR Channel
 *
 * It exposes the functionality to be expected for a channel
 *
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
          MemorySlot *const coordinationBuffer,
          const size_t tokenSize,
          const size_t capacity) :
           common::CircularBuffer
            (
             capacity,
             &_depth,
             &_tail
            ),
           _memoryManager(memoryManager),
           _tokenBuffer(tokenBuffer),
           _coordinationBuffer(coordinationBuffer),
           _poppedTokensSlot(_memoryManager->registerLocalMemorySlot(&_poppedTokens, sizeof(size_t))),
           _tokenSize(tokenSize)
  {
    if (_tokenSize == 0) HICR_THROW_LOGIC("Attempting to create a channel with token size 0.\n");
    if (_capacity == 0) HICR_THROW_LOGIC("Attempting to create a channel with zero capacity \n");
  }

  ~Base()
  {
    // Unregistering memory slot corresponding to popped token count
    _memoryManager->deregisterLocalMemorySlot(_poppedTokensSlot);
  }

  /**
   * This function updates the internal value of the channel depth
   */
  virtual void updateDepth() = 0;

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
  MemorySlot *const _coordinationBuffer;

  /**
   * Local memory slot to update the tail position
   */
  MemorySlot *const _poppedTokensSlot;

  /**
   * The number of popped tokens
   */
  size_t _poppedTokens = 0;

  /**
   * Internal counter for the number of pushed tokens by the producer
   */
  size_t _pushedTokens = 0;

  /**
   * Token size
   */
  const size_t _tokenSize;

  /**
   * Buffer position at the head
   */
  size_t _depth = 0;

  /**
   * Buffer position at the tail
   */
  size_t _tail = 0;
};

} // namespace SPSC

} // namespace channel

} // namespace HiCR
