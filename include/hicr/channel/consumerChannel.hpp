
/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file channel.hpp
 * @brief Provides functionality for a consumer channel over HiCR
 * @author A. N. Yzelman & S. M Martin
 * @date 15/9/2023
 */

#pragma once

#include <hicr/backend.hpp>
#include <hicr/channel/channel.hpp>
#include <hicr/common/definitions.hpp>
#include <hicr/common/exceptions.hpp>
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
   * \param[in] backend The backend that will facilitate communication between the producer and consumer sides
   * \param[in] tokenBuffer The memory slot pertaining to the token buffer. The producer will push new
   * tokens into this buffer, while there is enough space. This buffer should be big enough to hold at least one
   * token.
   * \param[in] coordinationBuffer This is a small buffer to enable the consumer to signal how many tokens it has
   * popped. It may also be used for other coordination signals.
   * \param[in] tokenSize The size of each token.
   */
  ConsumerChannel(Backend *backend,
    const Backend::memorySlotId_t tokenBuffer,
    const Backend::memorySlotId_t coordinationBuffer,
    const size_t tokenSize,
    const size_t capacity) :
     Channel(backend, tokenBuffer, coordinationBuffer, tokenSize, capacity) { }
  ~ConsumerChannel() = default;

  /**
   * Peeks in the local received queue and returns a pointer to the current
   * token.
   *
   * This is a one-sided blocking call that need not be made collectively.
   *
   * @param[in] n The number of tokens to peek.
   *
   * @returns If successful, a vector of size n with the relative positions within the token buffer of the received elements.
   * @returns If not successful, an empty vector
   *
   * This is a getter function that should complete in \f$ \Theta(n) \f$ time.
   * This function has one side-effect: it detects pending incoming messages and,
   * if there are, it updates the internal circular buffer with them.
   *
   * @see getDepth to determine whether the channel has an item to pop.
   *
   * \note While this function does not modify the state of the channel, the
   *       contents of the token may be modified by the caller.
   */
  __USED__ inline std::vector<size_t> peek(const size_t n = 1)
  {
    // We check only once for incoming messages (non-blocking operation)
    checkReceivedTokens();

    // Creating vector to store the token positions
    std::vector<size_t> positions;

    // If the exchange buffer does not have n tokens pushed, reject operation
    if (getDepth() < n) return positions;

    // Assigning the pointer to each token requested
    for (size_t i = 0; i < n; i++)
    {
      // Calculating buffer position
      size_t bufferPos = (getTailPosition() + i) % getCapacity();

      // Assigning offset to the output buffer
      positions.push_back(bufferPos);
    }

    // Succeeded in pushing the token(s)
    return positions;
  }

  /**
   * Similar to peek, but if the channel is empty, will wait until a new token
   * arrives.
   *
   * This is a one-sided blocking call that need not be made collectively.
   *
   * @param[in] n The number of tokens to peek.
   * @returns If successful, a vector of size n with the relative positions within the token buffer of the received elements.
   *
   * \warning This function may take an arbitrary amount of time and may, with
   *          incorrect usage, even result in deadlock. Always make sure to use
   *          this function in conjuction with e.g. SDF analysis to ensure no
   *          deadlock may occur. This type of analysis typically produces a
   *          minimum required channel capacity.
   *
   * \todo A preferred mechanism to wait for messages to have flushed may be
   *       the event-based API described below in this header file.
   */
  __USED__ inline std::vector<size_t> peekWait(const size_t n = 1)
  {
    // While the number of tokens in the buffer is less than the desired number, wait for it
    while (getDepth() < n) checkReceivedTokens();

    // Now do the peek, as designed above
    return peek(n);
  }

  /**
   * Removes the current token from the channel, and moves to the next token
   * (or to an empty channel state).
   *
   * This is a one-sided blocking call that need not be made collectively.
   *
   * @param[in] n How many tokens to pop. Optional; default is one.
   *
   * @returns <tt>true</tt>, if there were at least n tokens in the channel.
   * @returns <tt>false</tt>, otherwise.
   *
   * In case there are less than n tokens in the channel, no tokens will be popped.
   *
   * @see getDepth to determine whether the channel has an item to pop before calling
   * this function.
   */
  __USED__ inline bool pop(const size_t n = 1)
  {
    // If the exchange buffer does not have n tokens pushed, reject operation
    if (getDepth() < n) return false;

    // Advancing tail (removes elements from the circular buffer)
    advanceTail(n);

    // Notifying producer(s) of buffer liberation
    _backend->memcpy(_coordinationBuffer, 0, _tailPositionSlot, 0, sizeof(size_t));

    // If we reached this point, then the operation was successful
    return true;
  }

  private:

  __USED__ inline size_t getCurrentPushedTokenCount()
  {
   // Perform a non-blocking check of the token buffer, to see if there are new messages
   _backend->queryMemorySlotUpdates(_tokenBuffer);

   // Updating pushed tokens count
   auto pushedTokens = _backend->getMemorySlotReceivedMessages(_tokenBuffer);

   // Returning the number of pushed tokens
   return pushedTokens;
  }

  /**
   * This is a non-blocking non-collective function that requests the channel (and its underlying backend)
   * to check for the arrival of new messages. If this function is not called, then updates are not registered.
   *
   * @return The number of newly received tokens.
   */
  __USED__ inline size_t checkReceivedTokens()
  {
    // The number of received tokens is the difference between the currently pushed tokens and the previous one
    auto receivedTokens = getCurrentPushedTokenCount() - _pushedTokens;

    // We advance the head locally as many times as newly received tokens
    advanceHead(receivedTokens);

    // Returning the number of received tokens
    return receivedTokens;
  }

  private:

  /**
   * Internal counter for the number of pushed tokens by the producer
   */
  size_t _pushedTokens = 0;
};

}; // namespace HiCR
