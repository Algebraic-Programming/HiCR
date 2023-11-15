
/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file mpsc/producer.hpp
 * @brief Provides producer functionality for a Multiple-Producer Single-Consumer Channel over HiCR
 * @author S. M Martin
 * @date 14/11/2023
 */

#pragma once

#include <hicr/mpsc/base.hpp>
#include <hicr/common/definitions.hpp>
#include <hicr/common/exceptions.hpp>
#include <hicr/task.hpp>

namespace HiCR
{

namespace MPSC
{

/**
 * Class definition for a HiCR Producer MPSC Channel
 *
 * It exposes the functionality to be expected for a producer channel
 *
 */
class Producer final : public MPSC::Base
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
   * \param[in] coordinationBuffer This is a small buffer to enable the consumer to signal how many tokens it has
   *            popped. It may also be used for other coordination signals.
   * \param[in] tokenSize The size of each token.
   * \param[in] capacity The maximum number of tokens that will be held by this channel
   */
  Producer(backend::MemoryManager *memoryManager,
                  MemorySlot *const tokenBuffer,
                  MemorySlot *const localCoordinationBuffer,
                  MemorySlot *const globalCoordinationBuffer,
                  const size_t tokenSize,
                  const size_t capacity) :
                  MPSC::Base(memoryManager, tokenBuffer, localCoordinationBuffer, globalCoordinationBuffer, tokenSize, capacity)
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
  __USED__ inline bool push(MemorySlot *sourceSlot, const size_t n = 1)
  {
    // Make sure source slot is beg enough to satisfy the operation
    auto requiredBufferSize = getTokenSize() * n;
    auto providedBufferSize = sourceSlot->getSize();
    if (providedBufferSize < requiredBufferSize) HICR_THROW_LOGIC("Attempting to push with a source buffer size (%lu) smaller than the required size (Token Size (%lu) x n (%lu) = %lu).\n", providedBufferSize, getTokenSize(), n, requiredBufferSize);

    // Locking remote token and coordination buffer slots
    if (_memoryManager->acquireGlobalLock(_globalCoordinationBuffer) == false) return false;

    // Updating local coordination buffer
    _memoryManager->memcpy(_localCoordinationBuffer, 0, _globalCoordinationBuffer, 0, getCoordinationBufferSize());

    // If the exchange buffer does not have n free slots, reject the operation
    if (getDepth() + n > getCapacity()) { _memoryManager->releaseGlobalLock(_globalCoordinationBuffer); return false; }

    // Acquiring token slot lock
    _memoryManager->acquireGlobalLock(_tokenBuffer);

    // Copy tokens
    for (size_t i = 0; i < n; i++)
    {
      // Copying with source increasing offset per token
      _memoryManager->memcpy(_tokenBuffer, getTokenSize() * getHeadPosition(), sourceSlot, i * getTokenSize(), getTokenSize());

      // Advance head, as we have added a new element
      advanceHead(1);
    }

    // Updating global coordination buffer
    _memoryManager->memcpy(_globalCoordinationBuffer, 0, _localCoordinationBuffer, 0, getCoordinationBufferSize());

    // Adding flush operation to ensure buffers are ready for re-use
    _memoryManager->flush();

    // Acquiring token slot lock
    _memoryManager->releaseGlobalLock(_tokenBuffer);

    // Releasing remote token and coordination buffer slots
    _memoryManager->releaseGlobalLock(_globalCoordinationBuffer);

    // Succeeded
    return true;
  }
};

} // namespace MPSC

} // namespace HiCR
