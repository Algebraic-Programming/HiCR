
/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file variableSize/spsc/consumer.hpp
  @brief Provides functionality for a var-size SPSC consumer channel
 * @author K. Dichev & A. N. Yzelman & S. M Martin
 * @date 15/01/2024
 */

#pragma once

#include <frontends/channel/variableSize/base.hpp>
#include <array>
#include <numeric>
#include <cassert>

namespace HiCR
{

namespace channel
{

namespace variableSize
{

namespace SPSC
{

/**
 * Class definition for a HiCR Consumer Channel
 *
 * It exposes the functionality to be expected for a variable-size consumer channel
 *
 */
class Consumer final : public variableSize::Base
{
  protected:

  /**
   * pushed tokens is an incremental counter, used to find newly arrived message size metadata
   */
  size_t _pushedTokens;
  /**
   * pushed payloads is an incremental counter, used to find newly arrived messages
   */
  size_t _pushedPayloads;
  /**
   * pushed payload bytes is an incremental counter, used to set the head position
   */
  size_t _pushedPayloadBytes;
  /**
   * The global slot holding all payload data
   */
  std::shared_ptr<L0::GlobalMemorySlot> _payloadBuffer;
  /**
   * The total payload size (in bytes) of all elements currently in the buffer
   */
  size_t _payloadSize;

  /**
   * The memory slot pertaining to the local token buffer. It needs to be a global slot to enable the check
   * for updates from the remote producer. The token buffer is only used for metadata (payload message sizes) for
   * variable-sized consumer/producers
   */
  const std::shared_ptr<HiCR::L0::GlobalMemorySlot> _tokenBuffer;

  /**
   * The memory slot pertaining to the producer's message size information. This is a global slot to enable remote
   * update of the producer's internal circular buffer when doing a pop() operation
   */
  const std::shared_ptr<HiCR::L0::GlobalMemorySlot> _producerCoordinationBufferForCounts;

  /**
   * The memory slot pertaining to the producer's payload information. This is a global slot to enable remote
   * update of the producer's internal circular buffer when doing a pop() operation
   */
  const std::shared_ptr<HiCR::L0::GlobalMemorySlot> _producerCoordinationBufferForPayloads;

  public:

  /**
   * The constructor of the variable-sized consumer channel class.
   *
   * It requires the user to provide the allocated memory slots for the exchange (data) and coordination buffers.
   *
   * \param[in] communicationManager The backend to facilitate communication between the producer and consumer sides
   * \param[in] payloadBuffer The memory slot pertaining to the payload buffer. The producer will push new tokens
   * into this buffer, while there is enough space (in bytes). This buffer should be big enough to hold at least the
   * largest message of the variable-sized messages to be pushed.
   * \param[in] tokenBuffer The memory slot pertaining to the token buffer. This buffer is only used to exchange internal metadata
   * about the sizes of the individual messages being sent.
   * \param[in] internalCoordinationBufferForCounts This is a small buffer to hold the internal (local) state of the channel's circular buffer message counts
   * \param[in] internalCoordinationBufferForPayloads This is a small buffer to hold the internal (local) state of the channel's circular buffer payload sizes (in bytes)
   * \param[in] producerCoordinationBufferForCounts A global reference to the producer channel's internal coordination buffer for message counts, used for remote updates on pop()
   * \param[in] producerCoordinationBufferForPayloads A global reference to the producer channel's internal coordination buffer for payload sizes (in bytes), used for remote updates on pop()
   * \param[in] payloadCapacity The capacity (in bytes) of the buffer for variable-sized messages
   * \param[in] payloadSize The size of the payload datatype used to hold variable-sized messages of this datatype in the channel
   * \param[in] capacity The maximum number of tokens that will be held by this channel
   * @note: The token size in var-size channels is used only internally, and is passed as having a type size_t (with size sizeof(size_t))
   */
  Consumer(
    L1::CommunicationManager &communicationManager,
    std::shared_ptr<L0::GlobalMemorySlot> payloadBuffer,
    std::shared_ptr<L0::GlobalMemorySlot> tokenBuffer,
    std::shared_ptr<L0::LocalMemorySlot> internalCoordinationBufferForCounts,
    std::shared_ptr<L0::LocalMemorySlot> internalCoordinationBufferForPayloads,
    std::shared_ptr<L0::GlobalMemorySlot> producerCoordinationBufferForCounts,
    std::shared_ptr<L0::GlobalMemorySlot> producerCoordinationBufferForPayloads,
    const size_t payloadCapacity,
    const size_t payloadSize,
    const size_t capacity) : variableSize::Base(communicationManager, internalCoordinationBufferForCounts, internalCoordinationBufferForPayloads, capacity, payloadCapacity),
                             _pushedTokens(0),
                             _pushedPayloads(0),
                             _pushedPayloadBytes(0),
                             _payloadBuffer(payloadBuffer),
                             _payloadSize(payloadSize),
                             _tokenBuffer(tokenBuffer),
                             _producerCoordinationBufferForCounts(producerCoordinationBufferForCounts),
                             _producerCoordinationBufferForPayloads(producerCoordinationBufferForPayloads)
  {
    assert(internalCoordinationBufferForCounts != nullptr);
    assert(internalCoordinationBufferForPayloads != nullptr);
    assert(producerCoordinationBufferForCounts != nullptr);
    assert(producerCoordinationBufferForCounts != nullptr);
    _communicationManager->queryMemorySlotUpdates(_tokenBuffer);
    _communicationManager->queryMemorySlotUpdates(_payloadBuffer);
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
  __USED__ inline size_t basePeek(const size_t pos = 0)
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
   * This function gets the (starting position, size) pair for a given element in the consumer channel
   * @param[in] pos The payload position required. \p pos = 0 indicates earliest payload that
   *                is currently present in the buffer. \p pos = getDepth()-1 indicates
   *                the latest payload to have arrived.
   *
   * @returns A 2-element array:
   *  - the first value is the starting position in bytes of element \p pos in the payload buffer (in relation to starting address of payload buffer)
   *  - the second value is the size in bytes of element \p pos in the payload buffer.
   */
  __USED__ inline std::array<size_t, 2> peek(const size_t pos = 0)
  {
    if (pos != 0)
    {
      HICR_THROW_FATAL("peek only implemented for n = 0 at the moment!");
    }
    updateDepth();

    if (pos >= _circularBuffer->getDepth())
    {
      HICR_THROW_RUNTIME("Attempting to peek position (%lu) but not enough tokens (%lu) are in the buffer", pos, _circularBuffer->getDepth());
    }

    std::array<size_t, 2> result;
    result[0] = _circularBufferForPayloads->getTailPosition() % _circularBufferForPayloads->getCapacity();
    size_t *tokenBufferPtr = static_cast<size_t *>(_tokenBuffer->getSourceLocalMemorySlot()->getPointer());
    auto tokenPos = basePeek(pos);
    result[1] = tokenBufferPtr[tokenPos];

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
    size_t *tokenBufferPtr = static_cast<size_t *>(_tokenBuffer->getSourceLocalMemorySlot()->getPointer());

    size_t payloadBytes = 0;
    for (size_t i = 0; i < n; i++)
    {
      assert(i >= 0);
      size_t pos = basePeek(i);
      auto payloadSize = tokenBufferPtr[pos];
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
    size_t *tokenBufferPtr = static_cast<size_t *>(_tokenBuffer->getSourceLocalMemorySlot()->getPointer());
    size_t payloadBytes = 0;
    for (size_t i = 0; i < n; i++)
    {
      size_t ind = _circularBuffer->getDepth() - 1 - i;
      assert(ind >= 0);
      size_t pos = basePeek(ind);
      auto payloadSize = tokenBufferPtr[pos];
      payloadBytes += payloadSize;
    }

    return payloadBytes;
  }

  /**
   * This method updates the depth of both:
   * - message size metadata (in token slot)
   * - payload data (in payload)
   */
  __USED__ inline void updateDepth()
  {
    _communicationManager->queryMemorySlotUpdates(_tokenBuffer);
    _communicationManager->queryMemorySlotUpdates(_payloadBuffer);

    size_t newPushedTokens = _tokenBuffer->getSourceLocalMemorySlot()->getMessagesRecv() - _pushedTokens;
    size_t newPushedPayloads = _payloadBuffer->getSourceLocalMemorySlot()->getMessagesRecv() - _pushedPayloads;
    auto newTokensAndPayloads = std::min(newPushedTokens, newPushedPayloads);
    _pushedTokens += newTokensAndPayloads;
    _circularBuffer->setHead(_pushedTokens);
    _pushedPayloads += newTokensAndPayloads;
    auto newPayloadBytes = getNewPayloadBytes(newTokensAndPayloads);
    _pushedPayloadBytes += newPayloadBytes;

    _circularBufferForPayloads->setHead(_pushedPayloadBytes);
  }

  /**
   * Returns the current payload buffer depth, in bytes.
   *
   * If the current channel is a consumer, it corresponds to how many bytes
   * may yet be consumed. If the current channel is a producer, it corresponds
   * the channel capacity minus the returned value equals how many bytes may
   * still be pushed.
   *
   * \note This is not a thread-safe call
   *
   * This is a getter function that should complete in \f$ \Theta(1) \f$ time.
   *
   * @returns The bytes in the payload buffer of the channel
   *
   * This function when called on a valid channel instance will never fail.
   */
  __USED__ inline size_t getPayloadDepth()
  {
    return _circularBufferForPayloads->getDepth();
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
  __USED__ inline void pop(const size_t n = 1)
  {
    if (n > _circularBuffer->getCapacity()) HICR_THROW_LOGIC("Attempting to pop (%lu) tokens, which is larger than the channel capacity (%lu)", n, _circularBuffer->getCapacity());

    // Updating channel depth
    updateDepth();

    // If the exchange buffer does not have n tokens pushed, reject operation
    if (n > _circularBuffer->getDepth()) HICR_THROW_RUNTIME("Attempting to pop (%lu) tokens, which is more than the number of current tokens in the channel (%lu)", n, _circularBuffer->getDepth());
    auto payloadBytes = getOldPayloadBytes(n);
    _circularBuffer->advanceTail(n);
    _circularBufferForPayloads->advanceTail(payloadBytes);

    // Notifying producer(s) of buffer liberation
    _communicationManager->memcpy(_producerCoordinationBufferForCounts, 0, _coordinationBuffer, 0, 2 * sizeof(_HICR_CHANNEL_COORDINATION_BUFFER_ELEMENT_TYPE));
    _communicationManager->fence(_coordinationBuffer, 1, 0);
    _communicationManager->memcpy(_producerCoordinationBufferForPayloads, 0, _coordinationBufferForPayloads, 0, 2 * sizeof(_HICR_CHANNEL_COORDINATION_BUFFER_ELEMENT_TYPE));
    //_communicationManager->memcpy(_producerCoordinationBuffer, _HICR_CHANNEL_TAIL_ADVANCE_COUNT_IDX, _coordinationBuffer, _HICR_CHANNEL_TAIL_ADVANCE_COUNT_IDX, sizeof(_HICR_CHANNEL_COORDINATION_BUFFER_ELEMENT_TYPE));
    _communicationManager->fence(_coordinationBufferForPayloads, 1, 0);
  }

  /**
   * Returns the current variable-sized channel depth.
   *
   * If the current channel is a consumer, it corresponds to how many elements
   * may yet be consumed. If the current channel is a producer, it corresponds
   * the channel capacity minus the returned value equals how many elements may
   * still be pushed.
   *
   * \note This is not a thread-safe call
   *
   * This is a getter function that should complete in \f$ \Theta(1) \f$ time.
   *
   * @returns The number of elements in this channel.
   *
   * This function when called on a valid channel instance will never fail.
   */
  size_t getDepth()
  {
    // Because the current implementation first receives the message size in the token buffer, followed
    // by the message payload, it is possible for the token buffer to have a larged depth (by 1) than the payload buffer. Therefore, we need to return the minimum of the two depths
    return std::min(_circularBuffer->getDepth(), _circularBufferForPayloads->getDepth() / _payloadSize);
  }

  /**
   * This function can be used to quickly check whether the channel is empty.
   *
   * It does not affects the internal state of the channel
   *
   * \returns true, if both message count and payload buffers are empty
   * \returns false, if one of the buffers is not empty
   */
  bool isEmpty()
  {
    return (_circularBuffer->getDepth() == 0) && (_circularBufferForPayloads->getDepth() == 0);
  }

  /**
   * Retrieves the pointer to the channel's payload buffer
   *
   * @return The pointer to the payload buffer
   */
  std::shared_ptr<L0::GlobalMemorySlot> getPayloadBufferMemorySlot() const { return _payloadBuffer; }
};

} // namespace SPSC

} // namespace variableSize

} // namespace channel

} // namespace HiCR
