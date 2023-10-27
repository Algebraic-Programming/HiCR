
/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file consumerChannel.hpp
 * @brief Provides functionality for a consumer channel over HiCR
 * @author A. N. Yzelman & S. M Martin
 * @date 15/9/2023
 */

#pragma once

#include <hicr/channel/channel.hpp>
#include <hicr/common/definitions.hpp>
#include <hicr/common/exceptions.hpp>
#include <hicr/memorySlot.hpp>
#include <hicr/task.hpp>

namespace HiCR
{

/**
 * Class definition for a HiCR Consumer Channel
 *
 * It exposes the functionality to be expected for a consumer channel
 *
 */
class ConsumerChannel final : public Channel
{
  public:

  /**
   * The constructor of the consumer channel class.
   *
   * It requires the user to provide the allocated memory slots for the exchange (data) and coordination buffers.
   *
   * \param[in] memoryManager The backend to facilitate communication between the producer and consumer sides
   * \param[in] tokenBuffer The memory slot pertaining to the token buffer. The producer will push new
   * tokens into this buffer, while there is enough space. This buffer should be big enough to hold at least one
   * token.
   * \param[in] coordinationBuffer This is a small buffer to enable the consumer to signal how many tokens it has
   * popped. It may also be used for other coordination signals.
   * \param[in] tokenSize The size of each token.
   * \param[in] capacity The maximum number of tokens that will be held by this channel
   */
  ConsumerChannel(backend::MemoryManager *memoryManager,
                  MemorySlot *const tokenBuffer,
                  MemorySlot *const coordinationBuffer,
                  const size_t tokenSize,
                  const size_t capacity) : Channel(memoryManager, tokenBuffer, coordinationBuffer, tokenSize, capacity)
  {
    // Checking that the provided token exchange  buffer has the right size
    auto requiredTokenBufferSize = getTokenBufferSize(_tokenSize, _capacity);
    auto providedTokenBufferSize = _tokenBuffer->getSize();
    if (providedTokenBufferSize < requiredTokenBufferSize) HICR_THROW_LOGIC("Attempting to create a channel with a token data buffer size (%lu) smaller than the required size (%lu).\n", providedTokenBufferSize, requiredTokenBufferSize);
  }
  ~ConsumerChannel() = default;

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

  /**
   * Peeks in the local received queue and returns a pointer to the current
   * token.
   *
   * This is a one-sided blocking call that need not be made collectively.
   *
   * @param[in] pos The token position required. pos = 0 indicates earliest token that
   *                is currently present in the buffer. pos = getDepth()-1 indicates
   *                the latest token to have arrived.
   *
   * @returns A value representing the relative position within the token buffer where
   *         the required element can be found.
   *
   * This is a getter function that should complete in \f$ \Theta(1) \f$ time.
   * This function has no side-effects.
   *
   * An exception will occur if you do attempt to peek and no
   *
   * @see queryDepth to determine whether the channel has an item to pop.
   *
   * \note While this function does not modify the state of the channel, the
   *       contents of the token may be modified by the caller.
   */
  __USED__ inline size_t peek(const size_t pos = 0)
  {
    // Check if the requested position exceeds the capacity of the channel
    if (pos >= getCapacity()) HICR_THROW_LOGIC("Attempting to peek for a token with position (%lu), which is beyond than the channel capacity (%lu)", pos, getCapacity());

    // Updating channel depth
    updateDepth();

    // Check if there are enough tokens in the buffer to satisfy the request
    if (pos >= getDepth()) HICR_THROW_RUNTIME("Attempting to peek position (%lu) but not enough tokens (%lu) are in the buffer", pos, getDepth());

    // Calculating buffer position
    const size_t bufferPos = (getTailPosition() + pos) % getCapacity();

    // Succeeded in pushing the token(s)
    return bufferPos;
  }

  /**
   * Removes the current token from the channel, and moves to the next token
   * (or to an empty channel state).
   *
   * This is a one-sided blocking call that need not be made collectively.
   *
   * @param[in] n How many tokens to pop. Optional; default is one.
   *
   * In case there are less than n tokens in the channel, no tokens will be popped.
   *
   * @see queryDepth to determine whether the channel has an item to pop before calling
   * this function.
   */
  __USED__ inline void pop(const size_t n = 1)
  {
    if (n > getCapacity()) HICR_THROW_LOGIC("Attempting to pop (%lu) tokens, which is larger than the channel capacity (%lu)", n, getCapacity());

    // Updating channel depth
    updateDepth();

    // If the exchange buffer does not have n tokens pushed, reject operation
    if (n > getDepth()) HICR_THROW_RUNTIME("Attempting to pop (%lu) tokens, which is more than the number of current tokens in the channel (%lu)", n, getDepth());

    // Advancing tail (removes elements from the circular buffer)
    advanceTail(n);

    // Increasing the number of popped tokens
    _poppedTokens += n;

    // Notifying producer(s) of buffer liberation
    _memoryManager->memcpy(_coordinationBuffer, 0, _poppedTokensSlot, 0, sizeof(size_t));

    // Re-syncing token and coordination buffers
    _memoryManager->queryMemorySlotUpdates(_coordinationBuffer);
    _memoryManager->queryMemorySlotUpdates(_tokenBuffer);
  }

  /**
   * This function updates the internal value of the channel depth
   */
  __USED__ inline void updateDepth() override
  {
    checkReceivedTokens();
  }

  private:

  /**
   * This is a non-blocking non-collective function that requests the channel (and its underlying backend)
   * to check for the arrival of new messages. If this function is not called, then updates are not registered.
   */
  __USED__ inline void checkReceivedTokens()
  {
    // Perform a non-blocking check of the coordination and token buffers, to see and/or notify if there are new messages
    _memoryManager->queryMemorySlotUpdates(_tokenBuffer);

    // Updating pushed tokens count
    auto newPushedTokens = _tokenBuffer->getMessagesRecv();

    // The number of received tokens is the difference between the currently pushed tokens and the previous one
    auto receivedTokens = newPushedTokens - _pushedTokens;

    // We advance the head locally as many times as newly received tokens
    advanceHead(receivedTokens);

    // Update the number of pushed tokens
    _pushedTokens = newPushedTokens;
  }
};

}; // namespace HiCR
