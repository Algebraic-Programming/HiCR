
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

  ConsumerChannel(Backend* backend, Backend::memorySlotId_t exchangeBuffer, const size_t tokenSize) : Channel(backend, exchangeBuffer, tokenSize) {}
  ~ConsumerChannel() = default;

  /**
   * Peeks in the local received queue and returns a pointer to the current
   * token.
   *
   * This is a one-sided blocking call that need not be made collectively.
   *
   * This primitive may only be called by consumers.
   *
   * @param[out] token Storage onto which to copy the front data token (token)
   *
   * @returns <tt>true</tt> if the channel was non-empty.
   * @returns <tt>false</tt> if the channel was empty.
   *
   * This is a getter function that should complete in \f$ \Theta(1) \f$ time.
   *
   * \note While this function does not modify the state of the channel, the
   *       contents of the token may be modified by the caller.
   *
   * A call to this function throws an exception if:
   *  -# the channel at the current locality is a producer.
   */
  __USED__ inline bool peek(void *const &token) const
  {
   return false;
  }

  /**
   * Retrieves \a n tokens as a raw array.
   * @param[out] tokens Storage from which to read the input tokens
   * @param[in] n The batch size.
   * @see peek.
   *
   * A call to this function throws an exception if:
   *  -# the current #depth is less than \a n;
   *  -# the channel at the current locality is a producer.
   */
  __USED__ inline void peek(void *const &tokens, const size_t n) const
  {
  }


  /**
   * Similar to peek, but if the channel is empty, will wait until a new token
   * arrives.
   *
   * This is a one-sided blocking call that need not be made collectively.
   *
   * This primitive may only be called by consumers.
   *
   * @param[out] ptr  A pointer to the current token. Its value on input will
   *                  be ignored.
   * @param[out] size The size of the memory region pointed to. Its value on
   *                  input will be ignored.
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
   *  -# the channel at the current locality is a producer.
   */
  __USED__ inline void peek_wait(void *&ptr, size_t &size)
  {
  }

  /**
   * Removes the current token from the channel, and moves to the next token
   * (or to an empty channel state).
   *
   * This is a one-sided blocking call that need not be made collectively.
   *
   * This primitive may only be called by consumers.
   *
   * @param[in] n How many tokens to pop. Optional; default is one.
   *
   * @returns Whether the channel is empty after the current token has been
   *          removed.
   *
   * A call to this function throws an exception if:
   *  -# the channel at the current locality is a producer;
   *  -# the channel was empty.
   *
   * @see peek to determine whether the channel has an item to pop.
   */
  __USED__ inline bool pop(const size_t n = 1)
  {
   return false;
  }

};

}; // namespace HiCR
