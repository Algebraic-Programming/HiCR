
/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file fixedSize/mpsc/locking/consumer.hpp
 * @brief Provides Consumer functionality for a lock-based MPSC channel
 * @author S. M Martin, K. Dichev
 * @date 08/04/2024
 */

#pragma once

#include <hicr/core/definitions.hpp>
#include <hicr/core/exceptions.hpp>
#include <hicr/frontends/channel/fixedSize/base.hpp>

namespace HiCR
{

namespace channel
{

namespace fixedSize
{

namespace MPSC
{

namespace locking
{

/**
 * Class definition for a HiCR Consumer MPSC Channel
 *
 * It exposes the functionality to be expected for a consumer channel
 *
 */
class Consumer final : public channel::fixedSize::Base
{
  public:

  /**
   * The constructor of the consumer channel class.
   *
   * It requires the user to provide the allocated memory slots for the exchange (data) and coordination buffers.
   *
   * \param[in] communicationManager The backend to facilitate communication between the producer and consumer sides
   * \param[in] tokenBuffer The memory slot pertaining to the token buffer. The producer will push new
   * tokens into this buffer, while there is enough space. This buffer should be big enough to hold at least one
   * token.
   * \param[in] internalCoordinationBuffer This is a small buffer to hold the internal (loca) state of the channel's circular buffer
   * \param[in] consumerCoordinationBuffer A global reference to the channel's internal coordination buffer, used to check for updates (incoming messages)
   * \param[in] tokenSize The size of each token.
   * \param[in] capacity The maximum number of tokens that will be held by this channel
   */
  Consumer(L1::CommunicationManager             &communicationManager,
           std::shared_ptr<L0::GlobalMemorySlot> tokenBuffer,
           std::shared_ptr<L0::LocalMemorySlot>  internalCoordinationBuffer,
           std::shared_ptr<L0::GlobalMemorySlot> consumerCoordinationBuffer,
           const size_t                          tokenSize,
           const size_t                          capacity)
    : channel::fixedSize::Base(communicationManager, internalCoordinationBuffer, tokenSize, capacity),
      _consumerCoordinationBuffer(consumerCoordinationBuffer)
  {}
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
  __INLINE__ ssize_t peek(const size_t pos = 0)
  {
    // Check if the requested position exceeds the capacity of the channel
    if (pos >= _circularBuffer->getCapacity())
      HICR_THROW_LOGIC("Attempting to peek for a token with position %lu (token number %lu when starting from zero), which is beyond than the channel capacity (%lu)",
                       pos,
                       pos + 1,
                       _circularBuffer->getCapacity());

    // Value to return, initially set as -1 as default (not able to find the requested value)
    ssize_t bufferPos = -1;

    // Obtaining coordination buffer slot lock
    if (_communicationManager->acquireGlobalLock(_consumerCoordinationBuffer) == false) return bufferPos;

    _communicationManager->flushReceived();
    // Calculating current channel depth
    const auto curDepth = getDepth();

    // Calculating buffer position, if there are enough tokens in the buffer to satisfy the request
    if (pos < curDepth) bufferPos = (_circularBuffer->getTailPosition() + pos) % _circularBuffer->getCapacity();

    // Releasing coordination buffer slot lock
    _communicationManager->releaseGlobalLock(_consumerCoordinationBuffer);

    // Succeeded in pushing the token(s)
    return bufferPos;
  }

  /**
   * Removes the current token from the channel, and moves to the next token
   * (or to an empty channel state).
   *
   * This is a one-sided blocking call that need not be made collectively.
   *
   * In case there are less than n tokens in the channel, no tokens will be popped.
   *
   * @param[in] n How many tokens to pop. Optional; default is one.
   * @return True, if there was enough elements (>= n) to be removed; false, otherwise
   *
   * @see queryDepth to determine whether the channel has an item to pop before calling
   * this function.
   */
  __INLINE__ bool pop(const size_t n = 1)
  {
    if (n > _circularBuffer->getCapacity()) HICR_THROW_LOGIC("Attempting to pop %lu tokens, which is larger than the channel capacity (%lu)", n, _circularBuffer->getCapacity());

    // Flag to indicate whether the operaton was successful
    bool successFlag = false;

    // Obtaining coordination buffer slot lock
    if (_communicationManager->acquireGlobalLock(_consumerCoordinationBuffer) == false) return successFlag;

    // If the exchange buffer does not have n tokens pushed, reject operation, otherwise succeed
    if (n <= getDepth())
    {
      // Advancing tail (removes elements from the circular buffer)
      _circularBuffer->advanceTail(n);

      // Setting success flag
      successFlag = true;
    }

    // Releasing coordination buffer slot lock
    _communicationManager->releaseGlobalLock(_consumerCoordinationBuffer);

    // Operation was successful
    return successFlag;
  }

  private:

  /*
   * Global Memory slot pointing to the consumer's coordination buffer for acquiring a lock and updating
   */
  const std::shared_ptr<HiCR::L0::GlobalMemorySlot> _consumerCoordinationBuffer;
};

} // namespace locking

} // namespace MPSC

} // namespace fixedSize

} // namespace channel

} // namespace HiCR
