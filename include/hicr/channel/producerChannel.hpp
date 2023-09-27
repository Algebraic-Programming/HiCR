
/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file producerChannel.hpp
 * @brief Provides functionality for a producer channel over HiCR
 * @author A. N. Yzelman & S. M Martin
 * @date 28/7/2023
 */

#pragma once

#include <hicr/backend.hpp>
#include <hicr/channel/channel.hpp>
#include <hicr/common/definitions.hpp>
#include <hicr/common/exceptions.hpp>
#include <hicr/task.hpp>

namespace HiCR
{

/**
 * Class definition for a HiCR Producer Channel
 *
 * It exposes the functionality to be expected for a producer channel
 *
 */
class ProducerChannel final : public Channel
{
  public:

  /**
   * The constructor of the producer channel class.
   *
   * It requires the user to provide the allocated memory slots for the exchange (data) and coordination buffers.
   *
   * \param[in] backend The backend that will facilitate communication between the producer and consumer sides
   * popped. It may also be used for other coordination signals.
   * \param[in] tokenBuffer The memory slot pertaining to the token buffer. The producer will push new
   * tokens into this buffer, while there is enough space. This buffer should be big enough to hold at least one token.
   * \param[in] coordinationBuffer This is a small buffer to enable the consumer to signal how many tokens it has
   * \param[in] tokenSize The size of each token.
   * \param[in] capacity The maximum number of tokens that will be held by this channel
   */
  ProducerChannel(Backend *backend,
                  const Backend::memorySlotId_t tokenBuffer,
                  const Backend::memorySlotId_t coordinationBuffer,
                  const size_t tokenSize,
                  const size_t capacity) : Channel(backend, tokenBuffer, coordinationBuffer, tokenSize, capacity)
  {
    // Checking that the provided coordination buffer has the right size
    auto requiredCoordinationBufferSize = getCoordinationBufferSize();
    auto providedCoordinationBufferSize = _backend->getMemorySlotSize(_coordinationBuffer);
    if (providedCoordinationBufferSize < requiredCoordinationBufferSize) HICR_THROW_LOGIC("Attempting to create a channel with a coordination buffer size (%lu) smaller than the required size (%lu).\n", providedCoordinationBufferSize, requiredCoordinationBufferSize);
  }
  ~ProducerChannel() = default;

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
   * \param[in] backend The backend to perform the initialization operation with
   * \param[in] coordinationBuffer Memory slot corresponding to the coordination buffer
   */
  __USED__ static inline void initializeCoordinationBuffer(Backend *backend, const Backend::memorySlotId_t coordinationBuffer) noexcept
  {
    // Getting actual buffer of the coordination buffer
    auto buffer = backend->getLocalMemorySlotPointer(coordinationBuffer);

    // Resetting all its values to zero
    memset(buffer, 0, getCoordinationBufferSize());
  }

  /**
   * Puts new token(s) unto the channel.
   *
   * This is a one-sided blocking primitive that need not be made collectively.
   *
   * @param[in] sourceSlot  Source slot (buffer) from whence to read the token(s)
   * @param[in] n  Number of tokens to read from the buffer
   *
   * @returns <tt>true</tt>  If the channel had sufficient capacity for pushing
   *                         the token
   * @returns <tt>false</tt> If the channel did not have sufficient capacity.
   *                         In this case, the state of this channel shall be as
   *                         though this call to push had never occurred.
   *
   * A call to this function throws an exception if:
   *  -# the channel at this locality is a consumer.
   *
   * \internal This variant could be expressed as a call to the next one.
   */
  __USED__ inline bool push(const Backend::memorySlotId_t sourceSlot, const size_t n = 1)
  {
    // Make sure source slot is beg enough to satisfy the operation
    auto requiredBufferSize = getTokenSize() * n;
    auto providedBufferSize = _backend->getMemorySlotSize(sourceSlot);
    if (providedBufferSize < requiredBufferSize) HICR_THROW_LOGIC("Attempting to push with a source buffer size (%lu) smaller than the required size (Token Size (%lu) x n (%lu) = %lu).\n", providedBufferSize, getTokenSize(), n, requiredBufferSize);

    // Obtaining lock for thread safety
    _mutex.lock();

    // Getting result from calling pus
    const auto result = pushImpl(sourceSlot, n);

    // Releasing lock
    _mutex.unlock();

    // Succeeded in pushing the token(s)
    return result;
  }

  /**
   * Similar to push, but if the channel is full, will wait until outgoing
   * buffer space frees up.
   *
   * This is a one-sided blocking call that need not be made collectively.
   *
   * This function can only be called from the context of a running HiCR::Task,
   * because it is the only element in HiCR that can be freely suspended.
   *
   * @param[in] sourceSlot Memory slot from whence n tokens will be read and pushed into the channel
   * @param[in] n The number of n tokens to push into the channel
   *
   * \warning This function may take an arbitrary amount of time and may, with
   *          incorrect usage, even result in deadlock. Always make sure to use
   *          this function in conjuction with e.g. SDF analysis to ensure no
   *          deadlock may occur. This type of analysis typically produces a
   *          minimum required channel capacity.
   *
   * \todo A preferred mechanism to wait for messages to have flushed may be
   *       the event-based API described below in this header file.
   *
   * A call to this function throws an exception if:
   *  -# the \a slot, \a offset, \a size combination exceeds the memory region of \a slot.
   */
  __USED__ inline void pushWait(const Backend::memorySlotId_t sourceSlot, const size_t n = 1)
  {
    // Make sure source slot is beg enough to satisfy the operation
    auto requiredBufferSize = getTokenSize() * n;
    auto providedBufferSize = _backend->getMemorySlotSize(sourceSlot);
    if (providedBufferSize < requiredBufferSize) HICR_THROW_LOGIC("Attempting to push with a source buffer size (%lu) smaller than the required size (Token Size (%lu) x n (%lu) = %lu).\n", providedBufferSize, getTokenSize(), n, requiredBufferSize);

    // Obtaining lock for thread safety
    _mutex.lock();

    // Calling actual implementation
    pushWaitImpl(sourceSlot, n);

    // Releasing lock
    _mutex.unlock();
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
    _backend->queryMemorySlotUpdates(_coordinationBuffer);
    _backend->queryMemorySlotUpdates(_tokenBuffer);

    // Getting current tail position
    size_t currentPoppedTokens = _poppedTokens;

    // Updating local value of the tail until it changes
    _backend->memcpy(_poppedTokensSlot, 0, _coordinationBuffer, 0, sizeof(size_t));

    // Calculating difference between previous and new tail position
    size_t n = _poppedTokens - currentPoppedTokens;

    // Adjusting depth
    advanceTail(n);
  }

  __USED__ inline bool pushImpl(const Backend::memorySlotId_t sourceSlot, const size_t n)
  {
    // If the exchange buffer does not have n free slots, reject the operation
    if (getDepth() + n > getCapacity()) return false;

    // Copy tokens
    for (size_t i = 0; i < n; i++)
    {
      // Copying with source increasing offset per token
      _backend->memcpy(_tokenBuffer, getTokenSize() * getHeadPosition(), sourceSlot, i * getTokenSize(), getTokenSize());

      // Advance head, as we have added a new element
      advanceHead(1);
    }

    // Increasing the number of pushed tokens
    _pushedTokens += n;

    // Succeeded in pushing the token(s)
    return true;
  }

  __USED__ inline void pushWaitImpl(const Backend::memorySlotId_t sourceSlot, const size_t n)
  {
    // Copy tokens
    for (size_t i = 0; i < n; i++)
    {
      // If the exchange buffer is full, the task needs to be suspended until it's freed
      while (getDepth() == getCapacity()) checkReceiverPops();

      // Copying with source increasing offset per token
      _backend->memcpy(_tokenBuffer, getTokenSize() * getHeadPosition(), sourceSlot, i * getTokenSize(), getTokenSize());

      // Advance head, as we have added a new element
      advanceHead(1);
    }

    // Increasing the number of pushed tokens
    _pushedTokens += n;
  }
};

}; // namespace HiCR
