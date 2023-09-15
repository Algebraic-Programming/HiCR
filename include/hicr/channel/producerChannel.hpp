
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
  ProducerChannel(Backend* backend, Backend::memorySlotId_t exchangeBuffer, const size_t tokenSize) : Channel(backend, exchangeBuffer, tokenSize) {}
  ~ProducerChannel() = default;

  /**
   * Puts new token(s) unto the channel.
   *
   * This is a one-sided blocking primitive that need not be made collectively.
   *
   * This primitive may only be called by producers.
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
    // If the exchange buffer is full, reject the operation
    if (getDepth() + n > getCapacity()) return false;

    // Copy tokens
    for (size_t i = 0; i < n; i++)
    {
     // Copying with source increasing offset per token
     _backend->memcpy(_exchangeBuffer, getTokenSize() * getHeadPosition(), sourceSlot, i * getTokenSize(), getTokenSize(), _exchangeTag);

     // Advance head, as we have added a new element
     advanceHead(1);
    }

    // Succeeded in pushing the token(s)
    return true;
  }

  /**
   * Similar to push, but if the channel is full, will wait until outgoing
   * buffer space frees up.
   *
   * This is a one-sided blocking call that need not be made collectively.
   *
   * The primitive may only be called by producers.
   *
   * This function can only be called from the context of a running HiCR::Task,
   * because it is the only element in HiCR that can be freely suspended.
   *
   * @param[in] token  Input token to copy onto the channel
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
   *  -# the channel at this locality is a consumer;
   *  -# the \a slot, \a offset, \a size combination exceeds the memory region
   *     of \a slot.
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
    _backend->memcpy(_exchangeBuffer, getTokenSize() * getHeadPosition(), sourceSlot, i * getTokenSize(), getTokenSize(), _exchangeTag);

    // Advance head, as we have added a new element
    advanceHead(1);
   }
  }

  /**
   * Checks whether the receiver has freed up space in the receiver buffer
   * and reports how many tokens were popped.
   *
   * \internal This function needs to be re-callable without side-effects
   * since it will be called repeatedly to check whether a pending operation
   * has finished
   *
   * @return The number of tokens that were popped in the receiver side
   */
  __USED__ inline size_t checkReceiverPops()
  {
   // We have to figure out how to resolve this
   size_t poppedTokens = 0;

   // for each popped token, free up space in the circular buffer
   advanceTail(poppedTokens);

   return poppedTokens;
  }
};

}; // namespace HiCR
