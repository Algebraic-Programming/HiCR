
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

#include <hicr/common/definitions.hpp>
#include <hicr/common/exceptions.hpp>
#include <hicr/channel/spsc/base.hpp>

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
class Producer final : public SPSC::Base
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
                  MemorySlot *const coordinationBuffer,
                  const size_t tokenSize,
                  const size_t capacity)
  : SPSC::Base(memoryManager, tokenBuffer, coordinationBuffer, tokenSize, capacity)
  {
    // Checking that the provided coordination buffer has the right size
    auto requiredCoordinationBufferSize = getCoordinationBufferSize();
    auto providedCoordinationBufferSize = _coordinationBuffer->getSize();
    if (providedCoordinationBufferSize < requiredCoordinationBufferSize) HICR_THROW_LOGIC("Attempting to create a channel with a coordination buffer size (%lu) smaller than the required size (%lu).\n", providedCoordinationBufferSize, requiredCoordinationBufferSize);
  }
  ~Producer() = default;

  /**
   * This function can be used to check the size of the coordination buffer that needs to be provided
   * in the creation of the producer channel
   *
   * \return Size (bytes) of the coordination buffer
   */
  __USED__ static inline size_t getCoordinationBufferSize() noexcept
  {
    return sizeof(size_t);
  }

  /**
   * This function can be used to check the size of the coordination buffer that needs to be provided
   * in the creation of the producer channel
   *
   * \param[in] coordinationBuffer Memory slot corresponding to the coordination buffer
   */
  __USED__ static inline void initializeCoordinationBuffer(MemorySlot *coordinationBuffer)
  {
    // Checking for correct size
    auto requiredSize = getCoordinationBufferSize();
    auto size = coordinationBuffer->getSize();
    if (size < requiredSize) HICR_THROW_LOGIC("Attempting to initialize coordination buffer size on a memory slot (%lu) smaller than the required size (%lu).\n", size, requiredSize);

    // Getting actual buffer of the coordination buffer
    auto bufferPtr = coordinationBuffer->getPointer();

    // Resetting all its values to zero
    memset(bufferPtr, 0, getCoordinationBufferSize());
  }

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
  __USED__ inline void push(MemorySlot *sourceSlot, const size_t n = 1)
  {
    // Make sure source slot is beg enough to satisfy the operation
    auto requiredBufferSize = getTokenSize() * n;
    auto providedBufferSize = sourceSlot->getSize();
    if (providedBufferSize < requiredBufferSize) HICR_THROW_LOGIC("Attempting to push with a source buffer size (%lu) smaller than the required size (Token Size (%lu) x n (%lu) = %lu).\n", providedBufferSize, getTokenSize(), n, requiredBufferSize);

    // Updating channel depth
    updateDepth();

    // If the exchange buffer does not have n free slots, reject the operation
    if (getDepth() + n > getCapacity()) HICR_THROW_RUNTIME("Attempting to push with (%lu) tokens while the channel has (%lu) tokens and this would exceed capacity (%lu).\n", n, _depth, getCapacity());

    // Copy tokens
    for (size_t i = 0; i < n; i++)
    {
      // Copying with source increasing offset per token
      _memoryManager->memcpy(_tokenBuffer, getTokenSize() * getHeadPosition(), sourceSlot, i * getTokenSize(), getTokenSize());

      // Advance head, as we have added a new element
      advanceHead(1);
    }

    // Adding flush operation to ensure buffers are ready for re-use
    _memoryManager->flush();

    // Increasing the number of pushed tokens
    _pushedTokens += n;
  }

  /**
   * This function updates the internal value of the channel depth
   */
  __USED__ inline void updateDepth() override
  {
    checkReceiverPops();
  }

  private:

  /**
   * Checks whether the receiver has freed up space in the receiver buffer
   * and reports how many tokens were popped.
   *
   * \internal This function needs to be re-callable without side-effects
   * since it will be called repeatedly to check whether a pending operation
   * has finished.
   *
   * \internal This function relies on HiCR's one-sided communication semantics.
   * If the update of the popped tokens value required some kind of function call
   * in the backend, this will deadlock. To enable synchronized communication,
   * a call to Backend::queryMemorySlotUpdates should be added here.
   *
   */
  __USED__ inline void checkReceiverPops()
  {
    // Perform a non-blocking check of the coordination and token buffers, to see and/or notify if there are new messages
    _memoryManager->queryMemorySlotUpdates(_coordinationBuffer);

    // Getting current tail position
    size_t currentPoppedTokens = _poppedTokens;

    // Updating local value of the tail until it changes
    std::memcpy(_poppedTokensSlot->getPointer(), _coordinationBuffer->getPointer(), sizeof(size_t));

    // Calculating difference between previous and new tail position
    size_t n = _poppedTokens - currentPoppedTokens;

    // Adjusting depth
    advanceTail(n);
  }
};

} // namespace SPSC

} // namespace channel

} // namespace HiCR
