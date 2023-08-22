
/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file channel.hpp
 * @brief Provides various memcpy functions as well as corresponding fences.
 * @author A. N. Yzelman
 * @date 28/7/2023
 */

#pragma once

#include "dataMover.hpp"

namespace HiCR
{

/**
 * Class definition for a HiCR ChannelView
 *
 * It exposes the functionality to be expected for a channel
 *
 */
template <typename T>
class ChannelView
{
  protected:
  /**
   * A channel may only be instantiated via a call to
   * #MemorySpace::createChannel.
   */
  ChannelView() {}

  public:
  /**
   * Releases all resources corresponding to this channel, including allocated
   * buffer space as well as freeing up the \a tag used in construction for
   * other channels or memcpy requests.
   *
   * \internal In OO programming, standard practice is to declare destructors
   *           virtual.
   */
  virtual ~ChannelView() {}

  /**
   * @returns The capacity of the channel.
   *
   * This is a one-sided blocking call that need not be made collectively.
   *
   * This function when called on a valid channel instance will never fail.
   */
  size_t capacity() const noexcept;

  /**
   * @returns The number of elements in this channel.
   *
   * If the current channel is a consumer, it corresponds to how many tokens
   * may yet be consumed. If the current channel is a producer, it corresponds
   * the channel capacity minus the returned value equals how many tokens may
   * still be pushed.
   *
   * This is a one-sided blocking call that need not be made collectively.
   *
   * This is a getter function that should complete in \f$ \Theta(1) \f$ time.
   *
   * This function when called on a valid channel instance will never fail.
   */
  size_t depth() const noexcept;

  /**
   * Peeks in the local received queue and returns a pointer to the current
   * token.
   *
   * This is a one-sided blocking call that need not be made collectively.
   *
   * This primitive may only be called by consumers.
   *
   * @param[out] token Storage onto which to copy the front data element (token)
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
  bool peek(T &token) const;

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
  void peek(T *const &tokens, const size_t n) const;

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
  void peek_wait(void *&ptr, size_t &size);

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
   * @see ::peek to determine whether the channel has an item to pop.
   */
  bool pop(const size_t n = 1);

  /**
   * Puts a new token unto the channel.
   *
   * This is a one-sided blocking primitive that need not be made collectively.
   *
   * This primitive may only be called by producers.
   *
   * @param[in] token  Input token to copy onto the channel
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
  bool push(const T &token);

  /**
   * Batched version of #push.
   *
   * @param[in] tokens Raw array of tokens to push into the channel;
   * @param[in] n      The size of the raw array \a tokens.
   *
   * @returns How many tokens were successfully pushed into the channel.
   *
   * \note If no tokens were pushed, then \a n is returned.
   *
   * A call to this function throws an exception if:
   *  -# the channel at this locality is a consumer.
   *
   * \internal This variant could be expressed as a call to the next one.
   */
  size_t push(const T *const tokens, const size_t n);

  /**
   * Batched version of #push.
   *
   * @tparam RndAccIt A random access iterator over items of type \a T.
   *
   * @param[in] tokens Iterator over tokens to be pushed into the channel;
   * @param[in] tokens_end An iterator in end-position matching \a tokens.
   *
   * @returns How many tokens were successfully pushed into the channel.
   *
   * \note If no tokens were pushed, then \a n is returned, where \a n is
   *       \a tokens_end - \a tokens.
   *
   * A call to this function throws an exception if:
   *  -# the channel at this locality is a consumer.
   */
  template <typename RndAccIt>
  size_t push(RndAccIt tokens, const RndAccIt tokens_end);

  /**
   * Similar to push, but if the channel is full, will wait until outgoing
   * buffer space frees up.
   *
   * This is a one-sided blocking call that need not be made collectively.
   *
   * The primitive may only be called by producers.
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
  void push_wait(const T &token);

  // TODO register an effect somehow? Need to support two events:
  //   1) full-to-non-full (producer side),
  //   2) empty-to-nonempty (consumer side).
};

}; // namespace HiCR
