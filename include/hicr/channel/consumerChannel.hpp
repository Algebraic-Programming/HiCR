
/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file channel.hpp
 * @brief Provides functionality for a consumer channel over HiCR
 * @author A. N. Yzelman & S. M Martin
 * @date 15/9/2023
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
 * Class definition for a HiCR Consumer Channel
 *
 * It exposes the functionality to be expected for a consumer channel
 *
 */
class ConsumerChannel : public Channel
{
public:

  ConsumerChannel(Backend* backend, Backend::memorySlotId_t exchangeBuffer, Backend::memorySlotId_t coordinationBuffer, const size_t tokenSize) : Channel(backend, exchangeBuffer, coordinationBuffer, tokenSize),
  // Registering a slot for the local variable specifiying the nuber of popped tokens, to transmit it to the producer
  _poppedTokensSlot(backend->registerMemorySlot(&_poppedTokens, sizeof(size_t)))
  {}

  ~ConsumerChannel()
  {
   // Unregistering memory slot corresponding to popped token count
   _backend->deregisterMemorySlot(_poppedTokensSlot);
  }

  /**
   * Peeks in the local received queue and returns a pointer to the current
   * token.
   *
   * This is a one-sided blocking call that need not be made collectively.
   *
   * @param[out] ptrBuffer Storage onto which to copy the initial pointer for each of the n tokens to peek.
   * @param[in] n The number of tokens to peek.
   *
   * @returns <tt>true</tt> if the channel has n tokens in it.
   * @returns <tt>false</tt>, otherwise.
   *
   * This is a getter function that should complete in \f$ \Theta(n) \f$ time.
   *
   * @see getDepth to determine whether the channel has an item to pop.
   *
   * \note While this function does not modify the state of the channel, the
   *       contents of the token may be modified by the caller.
   */
  __USED__ inline bool peek(void** ptrBuffer, const size_t n = 1) const
  {
     // If the exchange buffer does not have n tokens pushed, reject operation
     if (getDepth() < n) return false;

     // Getting base pointer for the exchange buffer
     const auto basePtr = (uint8_t*) _backend->getMemorySlotPointer(_dataExchangeMemorySlot);

     // Assigning the pointer to each token requested
     for (size_t i = 0; i < n; i++)
     {
      // Calculating buffer position
      size_t bufferPos = (getTailPosition() + i) % getCapacity();

      // Calculating pointer to such position
      auto* tokenPtr = basePtr + bufferPos * getTokenSize();

      // Assigning pointer to the output buffer
      ptrBuffer[i] = tokenPtr;
     }

     // Succeeded in pushing the token(s)
     return true;
  }


  /**
   * Similar to peek, but if the channel is empty, will wait until a new token
   * arrives.
   *
   * This is a one-sided blocking call that need not be made collectively.
   *
   * @param[out] ptrBuffer Storage onto which to copy the initial pointer for each of the n tokens to peek.
   * @param[in] n The number of tokens to peek.
   *
   * \warning This function may take an arbitrary amount of time and may, with
   *          incorrect usage, even result in deadlock. Always make sure to use
   *          this function in conjuction with e.g. SDF analysis to ensure no
   *          deadlock may occur. This type of analysis typically produces a
   *          minimum required channel capacity.
   *
   * \todo A preferred mechanism to wait for messages to have flushed may be
   *       the event-based API described below in this header file.
   */
  __USED__ inline void peekWait(void** ptrBuffer, const size_t n = 1)
  {
   // This function can only be called from a running HiCR::Task
   if (_currentTask == NULL) HICR_THROW_LOGIC("ProducerChannel's peekWait function can only be called from inside the context of a running HiCR::Task\n");

   // While the number of tokens in the buffer is less than the desired number, wait for it
   while (getDepth() < n )
   {
    // Set the function that checks whether the buffer is now free to send more messages
    _currentTask->registerPendingOperation([this](){ return checkReceivedTokens() > 0; });

    // Suspend current task
    _currentTask->yield();
   }

   // Now do the peek, as designed above
   peek(ptrBuffer, n);
  }

  /**
   * Removes the current token from the channel, and moves to the next token
   * (or to an empty channel state).
   *
   * This is a one-sided blocking call that need not be made collectively.
   *
   * @param[in] n How many tokens to pop. Optional; default is one.
   *
   * @returns <tt>true</tt>, if there were at least n tokens in the channel.
   * @returns <tt>false</tt>, otherwise.
   *
   * In case there are less than n tokens in the channel, no tokens will be popped.
   *
   * @see getDepth to determine whether the channel has an item to pop before calling
   * this function.
   */
  __USED__ inline bool pop(const size_t n = 1)
  {
   // If the exchange buffer does not have n tokens pushed, reject operation
   if (getDepth() < n) return false;

   // Advancing tail (removes elements from the circular buffer)
   advanceTail(n);

   // Increasing the total number of popped tokens
   _poppedTokens += n;

   // Notifying producer(s) of buffer liberation
   _backend->memcpy(_coordinationMemorySlot, 0, _poppedTokensSlot, 0, sizeof(size_t));

   return false;
  }

  /**
   * This is a non-blocking non-collective function that requests the channel (and its underlying backend)
   * to check for the arrival of new messages. If this function is not called, then updates are not registered.
   *
   * @return The number of newly received tokens.
   */
  __USED__ inline size_t checkReceivedTokens()
  {
   // Perform a non-blocking check of the data exchange buffer, to see if there are new messages
   _backend->queryMemorySlotUpdates(_dataExchangeMemorySlot);

   // Temporarily storing the current received token (1 message = 1 token) count
   auto pushedTokensTmp = _pushedTokens;

   // Updating pushed tokens count
   _pushedTokens = _backend->getMemorySlotReceivedMessages(_dataExchangeMemorySlot);

   // The number of received tokens is the difference between the current pushed tokens and the previous one
   auto receivedTokens = _pushedTokens - pushedTokensTmp;

   // We advance the head locally as many times as newly received tokens
   advanceHead(receivedTokens);

   // Returning the number of received tokens
   return receivedTokens;
  }

private:

  /**
   * Local memory slot to update the number of popped tokens in the producer(s)
   */
  const Backend::memorySlotId_t _poppedTokensSlot;
};

}; // namespace HiCR
