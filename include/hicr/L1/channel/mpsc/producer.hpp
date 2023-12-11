
/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file mpsc/producer.hpp
 * @brief Provides producer functionality for a Multiple-Producer Single-Consumer Channel (MPSC) over HiCR
 * @author S. M Martin
 * @date 14/11/2023
 */

#pragma once

#include <hicr/common/definitions.hpp>
#include <hicr/common/exceptions.hpp>
#include <hicr/L1/channel/base.hpp>

namespace HiCR
{

namespace L1
{

namespace channel
{

namespace MPSC
{

/**
 * Class definition for a HiCR Producer MPSC Channel
 *
 * It exposes the functionality to be expected for a producer channel
 *
 */
class Producer final : public L1::channel::Base
{
  public:

  /**
   * The constructor of the producer channel class.
   *
   * It requires the user to provide the allocated memory slots for the exchange (data) and coordination buffers.
   *
   * \param[in] memoryManager The backend to facilitate communication between the producer and consumer sides
   * \param[in] tokenBuffer The memory slot pertaining to the token buffer. The producer will push new
   *            tokens into this buffer, while there is enough space. This buffer should be big enough to hold at least one token.
   * \param[in] producerCoordinationBuffer This is a small buffer to hold the internal state of the circular buffer
   * \param[in] consumerCoordinationBuffer This is a small buffer to hold the internal state of the circular buffer of the consumer.
   *            It needs to be a global reference to the remote consumer.
   * \param[in] tokenSize The size of each token.
   * \param[in] capacity The maximum number of tokens that will be held by this channel
   */
  Producer(L1::MemoryManager *memoryManager,
           L0::MemorySlot *const tokenBuffer,
           L0::MemorySlot *const producerCoordinationBuffer,
           L0::MemorySlot *const consumerCoordinationBuffer,
           const size_t tokenSize,
           const size_t capacity) : L1::channel::Base(memoryManager, tokenBuffer, producerCoordinationBuffer, tokenSize, capacity),
                                    _consumerCoordinationBuffer(consumerCoordinationBuffer)
  {
  }
  ~Producer() = default;

  /**
   * Puts new token(s) unto the channel.
   *
   * This is a one-sided blocking primitive that need not be made collectively.
   *
   * @param[in] sourceSlot  Source slot (buffer) from whence to read the token(s)
   * @param[in] n  Number of tokens to read from the buffer
   * @return True, if successful. False, if not (e.g., could not get ahold of the memory slot lock)
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
  __USED__ inline bool push(L0::MemorySlot *sourceSlot, const size_t n = 1)
  {
    // Make sure source slot is beg enough to satisfy the operation
    auto requiredBufferSize = getTokenSize() * n;
    auto providedBufferSize = sourceSlot->getSize();
    if (providedBufferSize < requiredBufferSize) HICR_THROW_LOGIC("Attempting to push with a source buffer size (%lu) smaller than the required size (Token Size (%lu) x n (%lu) = %lu).\n", providedBufferSize, getTokenSize(), n, requiredBufferSize);

    // Storage for operation success flag
    bool successFlag = false;

    // Locking remote token and coordination buffer slots
    if (_memoryManager->acquireGlobalLock(_consumerCoordinationBuffer) == false) return successFlag;

    // Updating local coordination buffer
    _memoryManager->memcpy(_coordinationBuffer, 0, _consumerCoordinationBuffer, 0, getCoordinationBufferSize());

    // Calculating current channel depth
    const auto depth = getDepth();

    // If the exchange buffer does not have n free slots, reject the operation
    if (depth + n <= getCapacity())
    {
      // Copying with source increasing offset per token
      for (size_t i = 0; i < n; i++) _memoryManager->memcpy(_tokenBuffer, getTokenSize() * getHeadPosition(), sourceSlot, i * getTokenSize(), getTokenSize());

      // Advance head, as we have added a new element
      advanceHead(n);

      // Updating global coordination buffer
      _memoryManager->memcpy(_consumerCoordinationBuffer, 0, _coordinationBuffer, 0, getCoordinationBufferSize());

      // Adding flush operation to ensure buffers are ready for re-use
      _memoryManager->flush();

      // Setting success flag as true
      successFlag = true;
    }

    // Releasing remote token and coordination buffer slots
    _memoryManager->releaseGlobalLock(_consumerCoordinationBuffer);

    // Succeeded
    return successFlag;
  }

  private:

  HiCR::L0::MemorySlot *const _consumerCoordinationBuffer;
};

} // namespace MPSC

} // namespace channel

} // namespace L1

} // namespace HiCR
