
/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file Provides various memcpy functions as well as corresponding fences.
 * @author A. N. Yzelman
 * @date 28/7/2023
 */

#pragma once

#include "datamover.hpp"

namespace HiCR {

class Channel {

 public:

  /**
   * Constructs a channel.
   *
   * @tparam SrcIt An iterator over HiCR MemorySpaces
   * @tparam DstIt An iterator over HiCR MemorySpaces
   *
   * Let \f$ S \f$ be a set of producers, and let \f$ D \f$ be a set of
   * consumers. \f$ S, D \f$ must contain at least one element. A channel
   * lets any producer put so-called \em tokens into a distributed buffer,
   * and lets any consumer retrieve tokens from that buffer.
   *
   * A channel is identified by an \a tag. The tag is a concept exposed by
   * associated memory spaces, and created via MemorySpace::createTagSlot.
   *
   * @param[in] producers_start An iterator in begin position to \f$ S \f$
   * @param[in] producers_end   An iterator in end position to \f$ S \f$
   * @param[in] consumers_start An iterator in begin position to \f$ D \f$
   * @param[in] consumers_end   An iterator in end position to \f$ D \f$
   * @param[in] capacity        How many tokens may be in the channel at
   *                            maximum, at any given time
   * @param[in] tag             The channel tag. Every channel should have a
   *                            unique tag associated to it.
   *
   * \warning Using the same \a tag for both a channel as well as individual
   *          calls to HiCR::memcpy or HiCR::fence will result in undefined
   *          behaviour.
   *
   * With normal semantics, a produced token ends up at one (out of potentially
   * many) consumers. This channel, however, includes a possibility where tokens
   * once submitted are broadcast to \em all consumers.
   *
   * @param[in] producersBroadcast Whether submitted tokens are to be broadcast
   *                               to all consumers.
   *
   * In broadcasting mode, broadcasting any single token to \f$ c \f$ consumers
   * counts as taking up \f$ c \f$ \a capacity.
   *
   * \note Rationale: in the worst case, the token ends up at \f$ c \f$
   *       receiving buffers without any one of them being consumed yet.
   *
   * A call to this constructor must be made collectively across all workers
   * that house the given resources. If the callee locality is in \f$ S \f$,
   * then the constructed channel is a producer. If the callee locality is in
   * \f$ D \f$, then the constructed channel is a consumer. A locality may
   * never be both in \f$ S \f$ \em \f$ D \f$.
   *
   * Channels always encapsulate one-copy communication. This means there is
   * always at least one copy of a token in either a sender or receiver buffer.
   *
   * \note For zero-copy communication mechanisms, see HiCR::memcpy.
   *
   * \todo This interface uses iterators instead of raw arrays for listing the
   *       producers and consumers. The memorySpace and datamover interfaces use
   *       raw arrays instead. We should probably just pick one API and make all
   *       APIs consistent with that choice.
   */
  template<typename SrcIt, typename DstIt>
  Channel(
   const SrcIt& producers_start, const SrcIt& producers_end,
   const DstIt& consumers_start, const DstIt& consumers_end,
   const size_t capacity,
   const TagSlot& tag,
   const bool producersBroadcast = false
  );

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
   * @param[out] ptr  A pointer to the current token. Its value on input will
   *                  be ignored.
   * @param[out] size The size of the memory region pointed to. Its value on
   *                  input will be ignored.
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
  bool peek(void * &ptr, size_t &size) const;

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
  void peek_wait(void * &ptr, size_t &size);

  /**
   * Removes the current token from the channel, and moves to the next token
   * (or to an empty channel state).
   *
   * This is a one-sided blocking call that need not be made collectively.
   *
   * This primitive may only be called by consumers.
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
  bool pop();

  /**
   * Puts a new token unto the channel.
   *
   * This is a one-sided blocking primitive that need not be made collectively.
   *
   * This primitive may only be called by producers.
   *
   * @param[in] slot   In which memory region the token resides
   * @param[in] offset At which offset within \a slot the token resides
   * @param[in] size   The size of the token within \a slot
   *
   * @returns <tt>true</tt>  If the channel had sufficient capacity for pushing
   *                         the token
   * @returns <tt>false</tt> If the channel did not have sufficient capacity.
   *                         In this case, the state of this channel shall be as
   *                         though this call to push had never occurred.
   *
   * A call to this function throws an exception if:
   *  -# the channel at this locality is a consumer;
   *  -# the \a slot, \a offset, \a size combination exceeds the memory region
   *     of \a slot.
   */
  bool push(const MemorySlot& slot, const size_t offset, const size_t size);

  /**
   * Similar to push, but if the channel is full, will wait until outgoing
   * buffer space frees up.
   *
   * This is a one-sided blocking call that need not be made collectively.
   *
   * The primitive may only be called by producers.
   *
   * @param[in] slot   In which memory region the token resides
   * @param[in] offset At which offset within \a slot the token resides
   * @param[in] size   The size of the token within \a slot
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
  void push_wait(const MemorySlot& slot, const size_t offset, const size_t size);

  // TODO register an effect somehow? Need to support two events:
  //   1) full-to-non-full (producer side),
  //   2) empty-to-nonempty (consumer side).

};

