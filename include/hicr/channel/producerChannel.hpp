
/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file channel.hpp
 * @brief Provides functionality for a producer channel over HiCR
 * @author A. N. Yzelman & S. M Martin
 * @date 28/7/2023
 */

#pragma once

#include <hicr/common/definitions.hpp>
#include <hicr/common/exceptions.hpp>
#include <hicr/channel/channel.hpp>
#include <hicr/backend.hpp>
#include <hicr/task.hpp>

namespace HiCR
{

/**
 * Class definition for a HiCR Producer Channel
 *
 * It exposes the functionality to be expected for a producer channel
 *
 */
class ProducerChannel : Channel
{
public:
  ProducerChannel(Backend* backend, Backend::memorySlotId_t exchangeBuffer, Backend::memorySlotId_t coordinationBuffer, const size_t tokenSize) :  Channel(backend, exchangeBuffer, coordinationBuffer, tokenSize),
    // Creating a memory slot for the local pointer of the _poppedTokens value, in order to update producers about the number of pops this consumer does
   _poppedTokensPointer ((size_t*)backend->getMemorySlotPointer(coordinationBuffer))
  {}
  ~ProducerChannel() = default;

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
  __USED__ inline bool push(Backend::memorySlotId_t sourceSlot, const size_t n = 1)
  {
    // If the exchange buffer does not have n free slots, reject the operation
    if (getDepth() + n > getCapacity()) return false;

    // Copy tokens
    for (size_t i = 0; i < n; i++)
    {
     // Copying with source increasing offset per token
     _backend->memcpy(_dataExchangeMemorySlot, getTokenSize() * getHeadPosition(), sourceSlot, i * getTokenSize(), getTokenSize());

     // Advance head, as we have added a new element
     advanceHead(1);
    }

    // Increase the counter of pushed tokens
    _pushedTokens += n;

    // Succeeded in pushing the token(s)
    return true;
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
  __USED__ inline void pushWait(Backend::memorySlotId_t sourceSlot, const size_t n = 1)
  {
   // This function can only be called from a running HiCR::Task
   if (_currentTask == NULL)  HICR_THROW_LOGIC("ProducerChannel's pushWait function can only be called from inside the context of a running HiCR::Task\n");

   // Copy tokens
   for (size_t i = 0; i < n; i++)
   {
    // If the exchange buffer is full, the task needs to be suspended until it's freed
    if (getDepth() == getCapacity())
    {
      // Set the function that checks whether the buffer is now free to send more messages
     _currentTask->registerPendingOperation([this](){ return checkReceiverPops() > 0; });

     // Suspend current task
     _currentTask->yield();
    }

    // Copying with source increasing offset per token
    _backend->memcpy(_dataExchangeMemorySlot, getTokenSize() * getHeadPosition(), sourceSlot, i * getTokenSize(), getTokenSize());

    // Advance head, as we have added a new element
    advanceHead(1);
   }

   // Increase the counter of pushed tokens
   _pushedTokens += n;
  }

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
   * @return The number of newly popped tokens in the receiver side
   */
  __USED__ inline size_t checkReceiverPops()
  {
   // Storing the current number of popped tokens
   auto currentPoppedTokens = _poppedTokens;

   // Update number of popped tokens
   _poppedTokens = *_poppedTokensPointer;

   // The number of newly popped tokens is the difference
   size_t newlyPoppedTokens = _poppedTokens - currentPoppedTokens;

   // for each popped token, free up space in the circular buffer
   advanceTail(newlyPoppedTokens);

   // Returning the number of newly popped tokens
   return newlyPoppedTokens;
  }

private:

  /**
   * This pointer represents the position in local memory where the producer can check
   * how many popped elements have been performed by the consumer. The pointer value is
   * determined at creation time
   */
  const size_t* _poppedTokensPointer;
};

}; // namespace HiCR
