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
 * @file variableSize/spsc/consumer.hpp
  @brief Provides functionality for a var-size SPSC consumer channel
 * @author K. Dichev & A. N. Yzelman & S. M Martin
 * @date 15/01/2024
 */

#pragma once

#include <array>
#include <numeric>
#include <cassert>
#include <hicr/frontends/channel/variableSize/base.hpp>
#include <utility>

namespace HiCR::channel::variableSize::SPSC
{

/**
 * Class definition for a HiCR Consumer Channel
 *
 * It exposes the functionality to be expected for a variable-size consumer channel
 *
 */
class Consumer final : public variableSize::Base
{
  public:

  /**
   * The constructor of the variable-sized consumer channel class.
   *
   * It requires the user to provide the allocated memory slots for the exchange (data) and coordination buffers.
   *
   * \param[in] communicationManager The backend to facilitate communication between the producer and consumer sides
   * \param[in] payloadBuffer The memory slot pertaining to the payload buffer. The producer will push new tokens
   *            into this buffer, while there is enough space (in bytes). This buffer should be big enough to hold at least the
   *            largest message of the variable-sized messages to be pushed.
   * \param[in] tokenBuffer The memory slot pertaining to the token buffer. This buffer is only used to exchange internal metadata
   *            about the sizes of the individual messages being sent.
   * \param[in] internalCoordinationBufferForCounts This is a small buffer to hold the internal (local) state of the
   *            channel's circular buffer message counts
   * \param[in] internalCoordinationBufferForPayloads This is a small buffer to hold the internal (local) state of the
   *            channel's circular buffer payload sizes (in bytes)
   * \param[in] producerCoordinationBufferForCounts A global reference to the producer channel's internal coordination
   *            buffer for message counts, used for remote updates on pop()
   * \param[in] producerCoordinationBufferForPayloads A global reference to the producer channel's internal coordination
   *            buffer for payload sizes (in bytes), used for remote updates on pop()
   * \param[in] payloadCapacity The capacity (in bytes) of the buffer for variable-sized messages
   * \param[in] capacity The maximum number of tokens that will be held by this channel
   * @note: The token size in var-size channels is used only internally, and is passed as having a type size_t (with size sizeof(size_t))
   */
  Consumer(CommunicationManager                    &communicationManager,
           std::shared_ptr<GlobalMemorySlot>        payloadBuffer,
           std::shared_ptr<GlobalMemorySlot>        tokenBuffer,
           const std::shared_ptr<LocalMemorySlot>  &internalCoordinationBufferForCounts,
           const std::shared_ptr<LocalMemorySlot>  &internalCoordinationBufferForPayloads,
           const std::shared_ptr<GlobalMemorySlot> &producerCoordinationBufferForCounts,
           std::shared_ptr<GlobalMemorySlot>        producerCoordinationBufferForPayloads,
           const size_t                             payloadCapacity,
           const size_t                             capacity)
    : variableSize::Base(communicationManager, internalCoordinationBufferForCounts, internalCoordinationBufferForPayloads, capacity, payloadCapacity),

      _payloadBuffer(std::move(payloadBuffer)),
      _tokenSizeBuffer(std::move(tokenBuffer)),
      _producerCoordinationBufferForCounts(producerCoordinationBufferForCounts),
      _producerCoordinationBufferForPayloads(std::move(producerCoordinationBufferForPayloads))
  {
    assert(internalCoordinationBufferForCounts != nullptr);
    assert(internalCoordinationBufferForPayloads != nullptr);
    assert(producerCoordinationBufferForCounts != nullptr);
    assert(producerCoordinationBufferForCounts != nullptr);
  }

  /**
   * Returns a pointer to the current token, which holds the payload size metadata here.
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
   * \note While this function does not modify the state of the channel, the
   *       contents of the token may be modified by the caller.
   */
  __INLINE__ size_t basePeek(const size_t pos = 0)
  {
    // Check if the requested position exceeds the capacity of the channel
    if (pos >= getCircularBufferForCounts()->getCapacity())
      HICR_THROW_LOGIC("Attempting to peek for a token with position (%lu), which is beyond than the channel capacity (%lu)", pos, getCircularBufferForCounts()->getCapacity());

    // Updating channel depth
    updateDepth();

    // Check if there are enough tokens in the buffer to satisfy the request
    if (pos >= getCircularBufferForCounts()->getDepth())
      HICR_THROW_RUNTIME("Attempting to peek position (%lu) but not enough tokens (%lu) are in the buffer", pos, getCircularBufferForCounts()->getDepth());

    // Calculating buffer position
    const size_t bufferPos = (getCircularBufferForCounts()->getTailPosition() + pos) % getCircularBufferForCounts()->getCapacity();

    // Succeeded in pushing the token(s)
    return bufferPos;
  }

  /**
   * This function gets the (starting position, size) pair for a given element in the consumer channel
   * @param[in] pos The payload position required. \p pos = 0 indicates earliest payload that
   *                is currently present in the buffer. \p pos = getDepth()-1 indicates
   *                the latest payload to have arrived.
   *
   * @returns A 2-element array:
   *  - the first value is the starting position in bytes of element \p pos in the payload buffer (in relation to starting address of payload buffer)
   *  - the second value is the size in bytes of element \p pos in the payload buffer.
   */
  __INLINE__ std::array<size_t, 2> peek(const size_t pos = 0)
  {
    if (pos != 0) { HICR_THROW_FATAL("peek only implemented for n = 0 at the moment!"); }
    updateDepth();

    if (pos >= getCircularBufferForCounts()->getDepth())
    {
      HICR_THROW_RUNTIME("Attempting to peek position (%lu) but not enough tokens (%lu) are in the buffer", pos, getCircularBufferForCounts()->getDepth());
    }

    std::array<size_t, 2> result{};
    result[0]              = getCircularBufferForPayloads()->getTailPosition() % getCircularBufferForPayloads()->getCapacity();
    size_t *tokenBufferPtr = static_cast<size_t *>(_tokenSizeBuffer->getSourceLocalMemorySlot()->getPointer());
    auto    tokenPos       = basePeek(pos);
    result[1]              = tokenBufferPtr[tokenPos];
    return result;
  }

  /**
   * This function inspects the oldest \n variable-sized elements in the token buffer to find how many bytes they occupy in the payload buffer
   * @param[in] n element count to inspect, starting from oldest, in the token buffer
   * @return total byte size that the oldest \n elements take in payload buffer
   */
  size_t getOldPayloadBytes(size_t n)
  {
    if (n == 0) return 0;
    size_t *tokenBufferPtr = static_cast<size_t *>(_tokenSizeBuffer->getSourceLocalMemorySlot()->getPointer());

    size_t payloadBytes = 0;
    for (size_t i = 0; i < n; i++)
    {
      assert(i >= 0);
      size_t pos         = basePeek(i);
      auto   payloadSize = tokenBufferPtr[pos];
      payloadBytes += payloadSize;
    }
    return payloadBytes;
  }

  /**
   * This function inspects the newest \n variable-sized elements in the token buffer to find how many bytes they occupy in the payload buffer
   * @param[in] n element count to inspect, starting from newest, in the token buffer
   * @return total byte size that the newest \n elements take in payload buffer
   */
  size_t getNewPayloadBytes(size_t n)
  {
    if (n == 0) return 0;
    size_t *tokenBufferPtr = static_cast<size_t *>(_tokenSizeBuffer->getSourceLocalMemorySlot()->getPointer());
    size_t  payloadBytes   = 0;
    for (size_t i = 0; i < n; i++)
    {
      size_t ind = getCircularBufferForCounts()->getDepth() - 1 - i;
      assert(ind >= 0);
      size_t pos         = basePeek(ind);
      auto   payloadSize = tokenBufferPtr[pos];
      payloadBytes += payloadSize;
    }

    return payloadBytes;
  }

  /**
   * Removes the last \n variable-sized elements from the payload buffer, and
   * the associated metadata in the token buffer.
   * This is a one-sided blocking call that need not be made collectively.
   *
   * @param[in] n How many variable-sized elements to pop. Optional; default is one.
   *
   * In case there are less than n elements in the channel, no elements will be popped.
   *
   */
  __INLINE__ void pop(const size_t n = 1)
  {
    if (n > getCircularBufferForCounts()->getCapacity())
      HICR_THROW_LOGIC("Attempting to pop (%lu) tokens, which is larger than the channel capacity (%lu)", n, getCircularBufferForCounts()->getCapacity());

    // Updating channel depth
    updateDepth();

    // If the exchange buffer does not have n tokens pushed, reject operation
    if (n > getCircularBufferForCounts()->getDepth())
      HICR_THROW_RUNTIME("Attempting to pop (%lu) tokens, which is more than the number of current tokens in the channel (%lu)", n, getCircularBufferForCounts()->getDepth());
    auto payloadBytes = getOldPayloadBytes(n);
    getCircularBufferForCounts()->advanceTail(n);
    getCircularBufferForPayloads()->advanceTail(payloadBytes);

    const auto coordBuffElemSize = sizeof(_HICR_CHANNEL_COORDINATION_BUFFER_ELEMENT_TYPE);
    // Notifying producer(s) of buffer liberation
    getCommunicationManager()->memcpy(_producerCoordinationBufferForCounts, /* destination */
                                      _HICR_CHANNEL_TAIL_ADVANCE_COUNT_IDX * coordBuffElemSize,
                                      getCoordinationBufferForCounts(),
                                      _HICR_CHANNEL_TAIL_ADVANCE_COUNT_IDX * coordBuffElemSize,
                                      coordBuffElemSize);

    getCommunicationManager()->memcpy(_producerCoordinationBufferForPayloads, /* destination */
                                      _HICR_CHANNEL_TAIL_ADVANCE_COUNT_IDX * coordBuffElemSize,
                                      getCoordinationBufferForPayloads(), /* source */
                                      _HICR_CHANNEL_TAIL_ADVANCE_COUNT_IDX * coordBuffElemSize,
                                      coordBuffElemSize);

    getCommunicationManager()->fence(getCoordinationBufferForCounts(), 1, 0);
    getCommunicationManager()->fence(getCoordinationBufferForPayloads(), 1, 0);
  }

  /**
   * In this implementation of SPSC, updateDepth for the consumer is a NOP.
   * Any push by the producer leads the producer to update the consumer head index
   * in a one-sided manner. The consumer does not need to update the depth for this
   * case.
   */
  __INLINE__ void updateDepth() {}

  /**
   * Get depth of variable-size consumer. Since we have 2 buffers - one
   * for counts, and one for payloads, we need to be careful.
   * Because the current implementation first receives the payload (phase 1) before
   * receiving the message counts (phase 2), returning this depth should guarantee
   * we already have received the payloads
   *
   * \note This is not a thread-safe call
   *
   * This is a getter function that should complete in \f$ \Theta(1) \f$ time.
   *
   * @return The number of elements in variable-size consumer channel
   */
  size_t getDepth() { return getCircularBufferForCounts()->getDepth(); }

  /**
   * Returns the current depth of the channel holding the payloads
   *
   * \note This is not a thread-safe call
   *
   * This is a getter function that should complete in \f$ \Theta(1) \f$ time.
   *
   * @returns The number of total bytes in the payloads channel
   *
   */
  size_t getPayloadDepth() { return getCircularBufferForPayloads()->getDepth(); }

  /**
   * This function can be used to quickly check whether the channel is empty.
   *
   * It does not affects the internal state of the channel
   *
   * \returns true, if both message count and payload buffers are empty
   * \returns false, if one of the buffers is not empty
   */
  bool isEmpty() { return (getDepth() == 0); }

  /**
   * Retrieves the pointer to the channel's payload buffer
   *
   * @return The pointer to the payload buffer
   */
  [[nodiscard]] std::shared_ptr<GlobalMemorySlot> getPayloadBufferMemorySlot() const { return _payloadBuffer; }

  private:

  /**
   * The global slot holding all payload data
   */
  std::shared_ptr<GlobalMemorySlot> _payloadBuffer;

  /**
   * The memory slot pertaining to the local token buffer. It needs to be a global slot to enable the check
   * for updates from the remote producer. The token buffer is only used for metadata (payload message sizes) for
   * variable-sized consumer/producers
   */
  const std::shared_ptr<GlobalMemorySlot> _tokenSizeBuffer;

  /**
   * The memory slot pertaining to the producer's message size information. This is a global slot to enable remote
   * update of the producer's internal circular buffer when doing a pop() operation
   */
  const std::shared_ptr<GlobalMemorySlot> _producerCoordinationBufferForCounts;

  /**
   * The memory slot pertaining to the producer's payload information. This is a global slot to enable remote
   * update of the producer's internal circular buffer when doing a pop() operation
   */
  const std::shared_ptr<GlobalMemorySlot> _producerCoordinationBufferForPayloads;
};

} // namespace HiCR::channel::variableSize::SPSC
