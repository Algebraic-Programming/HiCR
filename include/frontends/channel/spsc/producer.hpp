
/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file spsc/producer.hpp
 * @brief Provides producer functionality for a Single-Producer Single-Consumer channel (SPSC) over HiCR
 * @author A. N. Yzelman & S. M Martin
 * @date 28/7/2023
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
 * Class definition for a HiCR Producer Channel
 *
 * It exposes the functionality to be expected for a producer channel
 *
 */
class Producer final : public channel::Base
{
  public:

  /**
   * The constructor of the producer channel class.
   *
   * It requires the user to provide the allocated memory slots for the exchange (data) and coordination buffers.
   *
   * \param[in] communicationManager The backend to facilitate communication between the producer and consumer sides
   * \param[in] tokenBuffer The memory slot pertaining to the token buffer. The producer will push new
   *            tokens into this buffer, while there is enough space. This buffer should be big enough to hold at least one token.
   * \param[in] internalCoordinationBuffer This is a small buffer to hold the internal (loca) state of the channel's circular buffer
   * \param[in] producerCoordinationBuffer A global reference of the producer's own coordination buffer to check for pop updates produced by the remote consumer
   * \param[in] tokenSize The size of each token.
   * \param[in] capacity The maximum number of tokens that will be held by this channel
   */
  Producer(L1::CommunicationManager *communicationManager,
           L0::GlobalMemorySlot *const tokenBuffer,
           L0::LocalMemorySlot *const internalCoordinationBuffer,
           L0::GlobalMemorySlot *const producerCoordinationBuffer,
           const size_t tokenSize,
           const size_t capacity)
    : channel::Base(communicationManager, internalCoordinationBuffer, tokenSize, capacity),
      _tokenBuffer(tokenBuffer),
      _producerCoordinationBuffer(producerCoordinationBuffer) {}

  ~Producer() = default;

  /**
   * Puts new token(s) unto the channel.
   *
   * This is a one-sided blocking primitive that need not be made collectively.
   *
   * @param[in] sourceSlot  Source slot (buffer) from whence to read the token(s)
   * @param[in] n  Number of tokens to read from the buffer
   *
   * This operation will fail with an exception if:
   *  - The source buffer is smaller than required
   *  - The operation would exceed the buffer size
   *
   * A call to this function throws an exception if:
   *  -# the channel at this locality is a consumer.
   *
   * \internal This variant could be expressed as a call to the next one.
   */
  __USED__ inline void push(L0::LocalMemorySlot *sourceSlot, const size_t n = 1)
  {
    // Make sure source slot is beg enough to satisfy the operation
    auto requiredBufferSize = getTokenSize() * n;
    auto providedBufferSize = sourceSlot->getSize();
    if (providedBufferSize < requiredBufferSize) HICR_THROW_LOGIC("Attempting to push with a source buffer size (%lu) smaller than the required size (Token Size (%lu) x n (%lu) = %lu).\n", providedBufferSize, getTokenSize(), n, requiredBufferSize);

    // Updating channel depth
    updateDepth();

    // Calculating current channel depth
    auto curDepth = _circularBuffer->getDepth();

    // If the exchange buffer does not have n free slots, reject the operation
    if (curDepth + n > _circularBuffer->getCapacity()) HICR_THROW_RUNTIME("Attempting to push with (%lu) tokens while the channel has (%lu) tokens and this would exceed capacity (%lu).\n", n, curDepth, _circularBuffer->getCapacity());

    // Copying with source increasing offset per token
    for (size_t i = 0; i < n; i++) _communicationManager->memcpy(_tokenBuffer, getTokenSize() * _circularBuffer->getHeadPosition(), sourceSlot, i * getTokenSize(), getTokenSize());

    // Advance head, as we have added new elements
    _circularBuffer->advanceHead(n);

    // Adding flush operation to ensure buffers are ready for re-use
    _communicationManager->flush();
  }

  /**
   * This function updates the internal value of the channel depth
   */
  __USED__ inline void updateDepth()
  {
    // Perform a non-blocking check of the coordination and token buffers, to see and/or notify if there are new messages
    _communicationManager->queryMemorySlotUpdates(_producerCoordinationBuffer);
  }

  private:

  /**
   * Memory slot that represents the token buffer that producer sends data to
   */
  L0::GlobalMemorySlot *const _tokenBuffer;

  /*
   * Global Memory slot pointing to the producer's own coordination buffer
   */
  L0::GlobalMemorySlot *const _producerCoordinationBuffer;
};

} // namespace SPSC

} // namespace channel

} // namespace HiCR
