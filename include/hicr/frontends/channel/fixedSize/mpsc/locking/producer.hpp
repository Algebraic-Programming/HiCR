/*
 *   Copyright 2025 Huawei Technologies Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
  const std::shared_ptr<GlobalMemorySlot> _tokenBuffer;

  /*
   * Global Memory slot pointing to the consumer's coordination buffer for acquiring a lock and updating
   */
  const std::shared_ptr<HiCR::GlobalMemorySlot> _consumerCoordinationBuffer;

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
  Producer(CommunicationManager                   &communicationManager,
           std::shared_ptr<GlobalMemorySlot>       tokenBuffer,
           const std::shared_ptr<LocalMemorySlot> &internalCoordinationBuffer,
           std::shared_ptr<GlobalMemorySlot>       consumerCoordinationBuffer,
           const size_t                            tokenSize,
           const size_t                            capacity)
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
  __INLINE__ bool push(const std::shared_ptr<LocalMemorySlot> &sourceSlot, const size_t n = 1)
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
      // Copying with source increasing offset per token
      for (size_t i = 0; i < n; i++)
      {
        getCommunicationManager()->memcpy(_tokenBuffer,                                            /* destination */
                                          getTokenSize() * getCircularBuffer()->getHeadPosition(), /* dst_offset */
                                          sourceSlot,                                              /* source */
                                          i * getTokenSize(),                                      /* src_offset */
                                          getTokenSize());                                         /* size*/
        // Advance head here, since the memcpy relies on the up-to-date head position
        getCircularBuffer()->advanceHead(1);
      }

      getCommunicationManager()->fence(sourceSlot, n, 0);

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
