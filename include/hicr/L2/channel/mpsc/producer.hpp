
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

#include <hicr/L2/channel/base.hpp>
#include <hicr/common/definitions.hpp>
#include <hicr/common/exceptions.hpp>

namespace HiCR
{

namespace L2
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
class Producer final : public L2::channel::Base
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
   * \param[in] consumerCoordinationBuffer A global reference to the consumer channel's internal coordination buffer, used to push remote updates
   * \param[in] tokenSize The size of each token.
   * \param[in] capacity The maximum number of tokens that will be held by this channel
   */
  Producer(L1::CommunicationManager *communicationManager,
           L0::GlobalMemorySlot *const tokenBuffer,
           L0::LocalMemorySlot *const internalCoordinationBuffer,
           L0::GlobalMemorySlot *const consumerCoordinationBuffer,
           const size_t tokenSize,
           const size_t capacity) : L2::channel::Base(communicationManager, internalCoordinationBuffer, tokenSize, capacity),
                                    _tokenBuffer(tokenBuffer),
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
  __USED__ inline bool push(L0::LocalMemorySlot *sourceSlot, const size_t n = 1)
  {
    // Make sure source slot is beg enough to satisfy the operation
    auto requiredBufferSize = getTokenSize() * n;
    auto providedBufferSize = sourceSlot->getSize();
    if (providedBufferSize < requiredBufferSize) HICR_THROW_LOGIC("Attempting to push with a source buffer size (%lu) smaller than the required size (Token Size (%lu) x n (%lu) = %lu).\n", providedBufferSize, getTokenSize(), n, requiredBufferSize);

    // Flag to record whether the operation was successful or not (it simplifies code by releasing locks only once)
    bool successFlag = false;

    // Locking remote token and coordination buffer slots
    if (_communicationManager->acquireGlobalLock(_consumerCoordinationBuffer) == false) return successFlag;

    // Adding flush operation to ensure buffers are ready for re-use
    _communicationManager->flush();

    // Updating local coordination buffer
    _communicationManager->memcpy(_coordinationBuffer, 0, _consumerCoordinationBuffer, 0, getCoordinationBufferSize());

    // Calculating current channel depth
    const auto depth = getDepth();

    // If the exchange buffer does not have n free slots, reject the operation
    if (depth + n <= _circularBuffer->getCapacity())
    {
      // Copying with source increasing offset per token
      for (size_t i = 0; i < n; i++) _communicationManager->memcpy(_tokenBuffer, getTokenSize() * _circularBuffer->getHeadPosition(), sourceSlot, i * getTokenSize(), getTokenSize());

      // Advance head, as we have added a new element
      _circularBuffer->advanceHead(n);

      // Updating global coordination buffer
      _communicationManager->memcpy(_consumerCoordinationBuffer, 0, _coordinationBuffer, 0, getCoordinationBufferSize());

      // Adding flush operation to ensure buffers are ready for re-use
      _communicationManager->flush();

      // Mark operation as successful
      successFlag = true;
    }

    // Releasing remote token and coordination buffer slots
    _communicationManager->releaseGlobalLock(_consumerCoordinationBuffer);

    // Succeeded
    return successFlag;
  }

  private:

  /**
   * Memory slot that represents the token buffer that producer sends data to
   */
  L0::GlobalMemorySlot *const _tokenBuffer;

  /*
   * Global Memory slot pointing to the consumer's coordination buffer for acquiring a lock and updating
   */
  HiCR::L0::GlobalMemorySlot *const _consumerCoordinationBuffer;
};

} // namespace MPSC

} // namespace channel

} // namespace L2

} // namespace HiCR
