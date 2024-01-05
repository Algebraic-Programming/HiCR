
/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file spsc/consumer.hpp
 * @brief Provides consumer functionality for a Single-Producer Single-Consumer channel (SPSC) over HiCR
 * @author A. N. Yzelman & S. M Martin
 * @date 15/9/2023
 */

#pragma once

#include <hicr/definitions.hpp>
#include <hicr/exceptions.hpp>
#include <frontends/channel/base.hpp>

namespace HiCR
{

namespace channel
{

namespace SPSC
{

/**
 * Class definition for a HiCR Consumer Channel
 *
 * It exposes the functionality to be expected for a consumer channel
 *
 */
class Consumer final : public channel::Base
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
   * \param[in] producerCoordinationBuffer A global reference to the producer channel's internal coordination buffer, used for remote updates on pop()
   * \param[in] tokenSize The size of each token.
   * \param[in] capacity The maximum number of tokens that will be held by this channel
   */
  Consumer(L1::CommunicationManager &communicationManager,
           std::shared_ptr<L0::GlobalMemorySlot> tokenBuffer,
           std::shared_ptr<L0::LocalMemorySlot> internalCoordinationBuffer,
           std::shared_ptr<L0::GlobalMemorySlot> producerCoordinationBuffer,
           const size_t tokenSize,
           const size_t capacity) : channel::Base(communicationManager, internalCoordinationBuffer, tokenSize, capacity),
                                    _tokenBuffer(tokenBuffer),
                                    _producerCoordinationBuffer(producerCoordinationBuffer)

  {
    // Checking whether the memory slot is local. This backend only supports local data transfers
    if (tokenBuffer->getSourceLocalMemorySlot() == nullptr) HICR_THROW_LOGIC("The passed coordination slot was not created locally (it must be to be used internally by the channel implementation)\n");

    // Checking that the provided token exchange  buffer has the right size
    auto requiredTokenBufferSize = getTokenBufferSize(_tokenSize, capacity);
    auto providedTokenBufferSize = tokenBuffer->getSourceLocalMemorySlot()->getSize();
    if (providedTokenBufferSize < requiredTokenBufferSize) HICR_THROW_LOGIC("Attempting to create a channel with a token data buffer size (%lu) smaller than the required size (%lu).\n", providedTokenBufferSize, requiredTokenBufferSize);
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
  __USED__ inline size_t peek(const size_t pos = 0)
  {
    // Check if the requested position exceeds the capacity of the channel
    if (pos >= _circularBuffer->getCapacity()) HICR_THROW_LOGIC("Attempting to peek for a token with position (%lu), which is beyond than the channel capacity (%lu)", pos, _circularBuffer->getCapacity());

    // Updating channel depth
    updateDepth();

    // Check if there are enough tokens in the buffer to satisfy the request
    if (pos >= _circularBuffer->getDepth()) HICR_THROW_RUNTIME("Attempting to peek position (%lu) but not enough tokens (%lu) are in the buffer", pos, _circularBuffer->getDepth());

    // Calculating buffer position
    const size_t bufferPos = (_circularBuffer->getTailPosition() + pos) % _circularBuffer->getCapacity();

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
    if (n > _circularBuffer->getCapacity()) HICR_THROW_LOGIC("Attempting to pop (%lu) tokens, which is larger than the channel capacity (%lu)", n, _circularBuffer->getCapacity());

    // Updating channel depth
    updateDepth();

    // If the exchange buffer does not have n tokens pushed, reject operation
    if (n > _circularBuffer->getDepth()) HICR_THROW_RUNTIME("Attempting to pop (%lu) tokens, which is more than the number of current tokens in the channel (%lu)", n, _circularBuffer->getDepth());

    // Advancing tail (removes elements from the circular buffer)
    _circularBuffer->advanceTail(n);

    // Notifying producer(s) of buffer liberation
    _communicationManager->memcpy(_producerCoordinationBuffer, _HICR_CHANNEL_TAIL_ADVANCE_COUNT_IDX, _coordinationBuffer, _HICR_CHANNEL_TAIL_ADVANCE_COUNT_IDX, sizeof(_HICR_CHANNEL_COORDINATION_BUFFER_ELEMENT_TYPE));
  }

  /**
   * This function updates the internal value of the channel depth
   *
   * This is a non-blocking non-collective function that requests the channel (and its underlying backend)
   * to check for the arrival of new messages. If this function is not called, then updates are not registered.
   */
  __USED__ inline void updateDepth()
  {
    // Perform a non-blocking check of the coordination and token buffers, to see and/or notify if there are new messages
    _communicationManager->queryMemorySlotUpdates(_tokenBuffer);

    // Updating pushed tokens count
    auto receivedTokenCount = _tokenBuffer->getMessagesRecv();

    // We advance the head locally as many times as newly received tokens
    _circularBuffer->setHead(receivedTokenCount);
  }

  /**
   * The memory slot pertaining to the local token buffer. It needs to be a global slot to enable the check
   * for updates (received messages) from the remote producer.
   */
  const std::shared_ptr<HiCR::L0::GlobalMemorySlot> _tokenBuffer;

  /**
   * The memory slot pertaining to the producer's coordination buffer. This is a global slot to enable remote
   * update of the producer's internal circular buffer when doing a pop() operation
   */
  const std::shared_ptr<HiCR::L0::GlobalMemorySlot> _producerCoordinationBuffer;
};

} // namespace SPSC

} // namespace channel

} // namespace HiCR
