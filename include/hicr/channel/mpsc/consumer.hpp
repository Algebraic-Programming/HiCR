
/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file mpsc/consumer.hpp
 * @brief Provides Consumer functionality for a Multiple-Producer Single-Consumer Channel (MPSC) over HiCR
 * @author S. M Martin
 * @date 14/11/2023
 */

#pragma once

#include <hicr/common/definitions.hpp>
#include <hicr/common/exceptions.hpp>
#include <hicr/channel/mpsc/base.hpp>

namespace HiCR
{

namespace channel
{

namespace MPSC
{

/**
 * Class definition for a HiCR Consumer MPSC Channel
 *
 * It exposes the functionality to be expected for a consumer channel
 *
 */
class Consumer final : public MPSC::Base
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
  Consumer(backend::MemoryManager *memoryManager,
                  MemorySlot *const tokenBuffer,
                  MemorySlot *const localCoordinationBuffer,
                  MemorySlot *const globalCoordinationBuffer,
                  const size_t tokenSize,
                  const size_t capacity) : MPSC::Base(memoryManager, tokenBuffer, localCoordinationBuffer, globalCoordinationBuffer, tokenSize, capacity)
  {

  }
  ~Consumer() = default;


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
  __USED__ inline ssize_t peek(const size_t pos = 0)
  {
    // Check if the requested position exceeds the capacity of the channel
    if (pos >= getCapacity()) HICR_THROW_LOGIC("Attempting to peek for a token with position %lu (token number %lu when starting from zero), which is beyond than the channel capacity (%lu)", pos, pos+1, getCapacity());

    // Check if there are enough tokens in the buffer to satisfy the request
    if (pos >= getDepth()) return -1;

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
  __USED__ inline bool pop(const size_t n = 1)
  {
    if (n > getCapacity()) HICR_THROW_LOGIC("Attempting to pop %lu tokens, which is larger than the channel capacity (%lu)", n, getCapacity());

    // Obtaining coordination buffer slot lock
    if (_memoryManager->acquireGlobalLock(_globalCoordinationBuffer) == false) return false;

    // If the exchange buffer does not have n tokens pushed, reject operation
    if (n > getDepth()) { _memoryManager->releaseGlobalLock(_globalCoordinationBuffer); return false; }

    // Advancing tail (removes elements from the circular buffer)
    advanceTail(n);

    // Releasing coordination buffer slot lock
    _memoryManager->releaseGlobalLock(_globalCoordinationBuffer);

    // Operation was successful
    return true;
  }
};

} // namespace MPSC

} // namespace channel

} // namespace HiCR
