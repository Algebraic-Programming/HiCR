
/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file fixedSize/spsc/producer.hpp
 * @brief Provides producer functionality for a fixed-size SPSC channel over HiCR
 * @author A. N. Yzelman & S. M Martin & K. Dichev
 * @date 04/06/2024
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

namespace SPSC
{

/**
 * Class definition for a HiCR Producer Channel
 *
 * It exposes the functionality to be expected for a producer channel
 *
 */
class Producer : public fixedSize::Base
{
  private:

  /**
   * Memory slot that represents the token buffer that producer sends data to
   */
  const std::shared_ptr<L0::GlobalMemorySlot> _tokenBuffer;

  /*
   * Global Memory slot pointing to the consumer coordination buffer
   */
  const std::shared_ptr<L0::GlobalMemorySlot> _consumerCoordinationBuffer;

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
   * \param[in] consumerCoordinationBuffer A global reference to the consumer's coordination buffer in order to push tokens and update their head index
   *            produced by the remote consumer
   * \param[in] tokenSize The size of each token.
   * \param[in] capacity The maximum number of tokens that will be held by this channel
   */
  Producer(L1::CommunicationManager             &communicationManager,
           std::shared_ptr<L0::GlobalMemorySlot> tokenBuffer,
           std::shared_ptr<L0::LocalMemorySlot>  internalCoordinationBuffer,
           std::shared_ptr<L0::GlobalMemorySlot> consumerCoordinationBuffer,
           const size_t                          tokenSize,
           const size_t                          capacity)
    : fixedSize::Base(communicationManager, internalCoordinationBuffer, tokenSize, capacity),
      _tokenBuffer(tokenBuffer),
      _consumerCoordinationBuffer(consumerCoordinationBuffer)
  {}

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
  __INLINE__ void push(std::shared_ptr<L0::LocalMemorySlot> sourceSlot, const size_t n = 1)
  {
    // Make sure source slot is big enough to satisfy the operation
    auto requiredBufferSize = getTokenSize() * n;
    auto providedBufferSize = sourceSlot->getSize();
    if (providedBufferSize < requiredBufferSize)
      HICR_THROW_LOGIC("Attempting to push with a source buffer size (%lu) smaller than the required size (Token Size (%lu) x n (%lu) = %lu).\n",
                       providedBufferSize,
                       getTokenSize(),
                       n,
                       requiredBufferSize);

    // Updating channel depth
    updateDepth();

    // Calculating current channel depth
    auto curDepth = _circularBuffer->getDepth();

    // If the exchange buffer does not have n free slots, reject the operation
    if (curDepth + n > _circularBuffer->getCapacity())
      HICR_THROW_RUNTIME(
        "Attempting to push with (%lu) tokens while the channel has (%lu) tokens and this would exceed capacity (%lu).\n", n, curDepth, _circularBuffer->getCapacity());

    /**
     * Because it is possible that head advance (by producer)
     * and tail advance (signalled by consumer) overlap,
     * we allow for temporary illegal (tail > head) by using the
     * cached depth when advancing the head
     */
    _circularBuffer->setCachedDepth(curDepth);
    for (size_t i = 0; i < n; i++)
    {
      // Copying with source increasing offset per token
      _communicationManager->memcpy(_tokenBuffer,                                        /* destination */
                                    getTokenSize() * _circularBuffer->getHeadPosition(), /* dst_offset */
                                    sourceSlot,                                          /* source */
                                    i * getTokenSize(),                                  /* src_offset */
                                    getTokenSize());                                     /* size */
    }
    _communicationManager->fence(sourceSlot, n, 0);

    // read possibly slightly outdated depth here (will be updated next round)
    _circularBuffer->advanceHead(n, true);

    /*
     * In this implementation of producer-consumer, the producer
     * actively and in a one-sided manner updates the depth at the consumer.
     * This implementation has some advantages for MPSC implementations
     * on top of SPSC
     */
    _communicationManager->memcpy(_consumerCoordinationBuffer,
                                  _HICR_CHANNEL_HEAD_ADVANCE_COUNT_IDX * sizeof(size_t),
                                  _coordinationBuffer,
                                  _HICR_CHANNEL_HEAD_ADVANCE_COUNT_IDX * sizeof(size_t),
                                  sizeof(size_t));
    _communicationManager->fence(_coordinationBuffer, 1, 0);
  }

  /**
   * This function updates the internal value of the channel depth
   */
  __INLINE__ void updateDepth()
  {
    // Perform a non-blocking check of the coordination and token buffers, to see and/or notify if there are new messages
    _communicationManager->queryMemorySlotUpdates(_coordinationBuffer);
  }
};

} // namespace SPSC

} // namespace fixedSize

} // namespace channel

} // namespace HiCR
