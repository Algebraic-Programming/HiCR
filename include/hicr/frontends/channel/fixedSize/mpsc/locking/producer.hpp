
/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file fixedSize/mpsc/locking/producer.hpp
 * @brief Provides producer functionality for a lock-based MPSC Channel
 * @author S. M Martin, K. Dichev
 * @date 08/04/2024
 */

#pragma once

#include <hicr/core/definitions.hpp>
#include <hicr/core/exceptions.hpp>
#include <hicr/frontends/channel/fixedSize/base.hpp>
#include <utility>

namespace HiCR::channel::fixedSize::MPSC::locking
{

/**
 * Class definition for a HiCR Producer MPSC Channel
 *
 * It exposes the functionality to be expected for a producer channel
 *
 */
class Producer final : public fixedSize::Base
{
  private:

  /**
   * Memory slot that represents the token buffer that producer sends data to
   */
  const std::shared_ptr<L0::GlobalMemorySlot> _tokenBuffer;

  /*
   * Global Memory slot pointing to the consumer's coordination buffer for acquiring a lock and updating
   */
  const std::shared_ptr<HiCR::L0::GlobalMemorySlot> _consumerCoordinationBuffer;

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
  Producer(L1::CommunicationManager                   &communicationManager,
           std::shared_ptr<L0::GlobalMemorySlot>       tokenBuffer,
           const std::shared_ptr<L0::LocalMemorySlot> &internalCoordinationBuffer,
           std::shared_ptr<L0::GlobalMemorySlot>       consumerCoordinationBuffer,
           const size_t                                tokenSize,
           const size_t                                capacity)
    : fixedSize::Base(communicationManager, internalCoordinationBuffer, tokenSize, capacity),
      _tokenBuffer(std::move(tokenBuffer)),
      _consumerCoordinationBuffer(std::move(consumerCoordinationBuffer))
  {}
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
  __INLINE__ bool push(const std::shared_ptr<L0::LocalMemorySlot> &sourceSlot, const size_t n = 1)
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

    // Flag to record whether the operation was successful or not (it simplifies code by releasing locks only once)
    bool successFlag = false;

    // Locking remote token and coordination buffer slots
    if (getCommunicationManager()->acquireGlobalLock(_consumerCoordinationBuffer) == false) return successFlag;

    // Updating local coordination buffer
    getCommunicationManager()->memcpy(getCoordinationBuffer(), 0, _consumerCoordinationBuffer, 0, getCoordinationBufferSize());

    // Adding fence operation to ensure buffers are ready for re-use
    getCommunicationManager()->fence(getCoordinationBuffer(), 0, 1);

    // Calculating current channel depth
    const auto depth = getDepth();

    // If the exchange buffer does not have n free slots, reject the operation
    if (depth + n <= getCircularBuffer()->getCapacity())
    {
      // setting the cached depth is done to avoid the situation
      // when the effect of a consumer popping an element is pushed to the producer
      // after the consumer releases the lock and after the producer acquires it
      // for a push, resulting in temporary illegal depth of the circular buffer
      getCircularBuffer()->setCachedDepth(depth);

      // Copying with source increasing offset per token
      for (size_t i = 0; i < n; i++)
      {
        getCommunicationManager()->memcpy(_tokenBuffer,                                            /* destination */
                                          getTokenSize() * getCircularBuffer()->getHeadPosition(), /* dst_offset */
                                          sourceSlot,                                              /* source */
                                          i * getTokenSize(),                                      /* src_offset */
                                          getTokenSize());                                         /* size*/
      }
      getCommunicationManager()->fence(sourceSlot, n, 0);

      // Advance head, as we have added n new elements
      getCircularBuffer()->advanceHead(n, true);

      // Updating global coordination buffer
      getCommunicationManager()->memcpy(_consumerCoordinationBuffer, 0, getCoordinationBuffer(), 0, getCoordinationBufferSize());
      // Adding fence operation to ensure buffers are ready for re-use
      getCommunicationManager()->fence(getCoordinationBuffer(), 1, 0);

      // Mark operation as successful
      successFlag = true;
    }

    // Releasing remote token and coordination buffer slots
    getCommunicationManager()->releaseGlobalLock(_consumerCoordinationBuffer);

    // Succeeded
    return successFlag;
  }
};

} // namespace HiCR::channel::fixedSize::MPSC::locking
