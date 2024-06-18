
/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file variableSize/mpsc/locking/consumer.hpp
 * @brief Provides variable-sized MPSC consumer channel, locking version 
 * @author O. Korakitis & K. Dichev
 * @date 11/03/2024
 */

#pragma once

#include <hicr/frontends/channel/variableSize/base.hpp>

namespace HiCR
{

namespace channel
{

namespace variableSize
{

namespace MPSC
{

namespace locking
{

/**
 * Class definition for a HiCR MPSC Consumer Channel
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
  const std::shared_ptr<L0::GlobalMemorySlot> _tokenSizeBuffer;

  /**
   * The memory slot pertaining to the consumer's message size information. This is a global slot to enable remote
   * update of the producer's internal circular buffer when doing a pop() operation
   */
  const std::shared_ptr<L0::GlobalMemorySlot> _consumerCoordinationBufferForCounts;

  /**
   * The memory slot pertaining to the consumer's payload information. This is a global slot to enable remote
   * update of the producer's internal circular buffer when doing a pop() operation
   */
  const std::shared_ptr<L0::GlobalMemorySlot> _consumerCoordinationBufferForPayloads;

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
   * \param[in] consumerCoordinationBufferForCounts A global reference to the producer channel's internal coordination
   *            buffer for message counts, used for remote updates on pop()
   * \param[in] consumerCoordinationBufferForPayloads A global reference to the producer channel's internal coordination
   *            buffer for payload sizes (in bytes), used for remote updates on pop()
   * \param[in] payloadCapacity The capacity (in bytes) of the buffer for variable-sized messages
   * \param[in] payloadSize The size of the payload datatype used to hold variable-sized messages of this datatype in the channel
   * \param[in] capacity The maximum number of tokens that will be held by this channel
   * @note: The token size in var-size channels is used only internally, and is passed as having a type size_t (with size sizeof(size_t))
   */
  Consumer(L1::CommunicationManager             &communicationManager,
           std::shared_ptr<L0::GlobalMemorySlot> payloadBuffer,
           std::shared_ptr<L0::GlobalMemorySlot> tokenBuffer,
           std::shared_ptr<L0::LocalMemorySlot>  internalCoordinationBufferForCounts,
           std::shared_ptr<L0::LocalMemorySlot>  internalCoordinationBufferForPayloads,
           std::shared_ptr<L0::GlobalMemorySlot> consumerCoordinationBufferForCounts,
           std::shared_ptr<L0::GlobalMemorySlot> consumerCoordinationBufferForPayloads,
           const size_t                          payloadCapacity,
           const size_t                          payloadSize,
           const size_t                          capacity)
    : variableSize::Base(communicationManager, internalCoordinationBufferForCounts, internalCoordinationBufferForPayloads, capacity, payloadCapacity),
      _pushedTokens(0),
      _pushedPayloads(0),
      _pushedPayloadBytes(0),
      _payloadBuffer(payloadBuffer),
      _payloadSize(payloadSize),
      _tokenSizeBuffer(tokenBuffer),
      _consumerCoordinationBufferForCounts(consumerCoordinationBufferForCounts),
      _consumerCoordinationBufferForPayloads(consumerCoordinationBufferForPayloads)
  {
    assert(internalCoordinationBufferForCounts != nullptr);
    assert(internalCoordinationBufferForPayloads != nullptr);
    assert(consumerCoordinationBufferForCounts != nullptr);
    assert(consumerCoordinationBufferForCounts != nullptr);
    _communicationManager->queryMemorySlotUpdates(_tokenSizeBuffer->getSourceLocalMemorySlot());
    _communicationManager->queryMemorySlotUpdates(_payloadBuffer->getSourceLocalMemorySlot());
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
    if (pos >= _circularBufferForCounts->getCapacity())
      HICR_THROW_LOGIC("Attempting to peek for a token with position (%lu), which is beyond than the channel capacity (%lu)", pos, _circularBufferForCounts->getCapacity());

    // Check if there are enough tokens in the buffer to satisfy the request
    if (pos >= _circularBufferForCounts->getDepth())
      HICR_THROW_RUNTIME("Attempting to peek position (%lu) but not enough tokens (%lu) are in the buffer", pos, _circularBufferForCounts->getDepth());

    // Calculating buffer position
    const size_t bufferPos = (_circularBufferForCounts->getTailPosition() + pos) % _circularBufferForCounts->getCapacity();

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
   *  - the first value is the starting position in bytes of element \p pos in the payload buffer
   *    (in relation to starting address of payload buffer)
   *  - the second value is the size in bytes of element \p pos in the payload buffer.
   *
   *  \note This peek does not grab the global lock, unlike e.g. fixedSize::mpsc::Consumer::peek.
   */
  __INLINE__ std::array<size_t, 2> peek(const size_t pos = 0)
  {
    _communicationManager->flushReceived();
    std::array<size_t, 2> result;
    if (pos != 0) { HICR_THROW_FATAL("peek only implemented for n = 0 at the moment!"); }
    if (pos >= _circularBufferForCounts->getDepth())
    {
      HICR_THROW_RUNTIME("Attempting to peek position (%lu) but not enough tokens (%lu) are in the buffer", pos, _circularBufferForCounts->getDepth());
    }

    result[0]              = _circularBufferForPayloads->getTailPosition() % _circularBufferForPayloads->getCapacity();
    size_t *tokenBufferPtr = static_cast<size_t *>(_tokenSizeBuffer->getSourceLocalMemorySlot()->getPointer());
    auto    tokenPos       = basePeek(pos);
    result[1]              = tokenBufferPtr[tokenPos];
    return result;
  }

  /**
   * This function inspects the oldest \n variable-sized elements in the token buffer to find how many bytes they
   * occupy in the payload buffer
   * @param[in] n element count to inspect, starting from oldest, in the token buffer
   * @return total byte size that the oldest \n elements take in payload buffer
   */
  __INLINE__ size_t getOldPayloadBytes(size_t n)
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
  __INLINE__ size_t getNewPayloadBytes(size_t n)
  {
    if (n == 0) return 0;
    size_t *tokenBufferPtr = static_cast<size_t *>(_tokenSizeBuffer->getSourceLocalMemorySlot()->getPointer());
    size_t  payloadBytes   = 0;

    for (size_t i = 0; i < n; i++)
    {
      size_t ind = _circularBufferForCounts->getDepth() - 1 - i;
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
   * @return True, if the lock was acquired AND there were enough elements (>= n) to be removed; false, otherwise
   *
   * In case there are less than n elements in the channel, no elements will be popped.
   *
   */
  __INLINE__ bool pop(const size_t n = 1)
  {
    bool successFlag = false;

    // Locking remote coordination buffer slot
    if (_communicationManager->acquireGlobalLock(_consumerCoordinationBufferForCounts) == false) return successFlag;

    if (n > _circularBufferForCounts->getCapacity())
      HICR_THROW_LOGIC("Attempting to pop (%lu) tokens, which is larger than the channel capacity (%lu)", n, _circularBufferForCounts->getCapacity());
    // If the exchange buffer does not have n tokens pushed, reject operation
    if (n > _circularBufferForCounts->getDepth())
      HICR_THROW_RUNTIME("Attempting to pop (%lu) tokens, which is more than the number of current tokens in the channel (%lu)", n, _circularBufferForCounts->getDepth());

    size_t *tokenBufferPtr = static_cast<size_t *>(_tokenSizeBuffer->getSourceLocalMemorySlot()->getPointer());

    size_t bytesOldestEntry = tokenBufferPtr[_circularBufferForCounts->getTailPosition()];

    _circularBufferForCounts->advanceTail(n);
    _circularBufferForPayloads->advanceTail(bytesOldestEntry);

    _communicationManager->releaseGlobalLock(_consumerCoordinationBufferForCounts);
    successFlag = true;
    return successFlag;
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
  size_t getDepth() { return _circularBufferForCounts->getDepth(); }

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
  std::shared_ptr<L0::GlobalMemorySlot> getPayloadBufferMemorySlot() const { return _payloadBuffer; }
};

} // namespace locking

} // namespace MPSC

} // namespace variableSize

} // namespace channel

} // namespace HiCR
