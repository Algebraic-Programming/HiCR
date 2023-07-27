
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
   * @tparam SrcIt An iterator over HiCR resources
   * @tparam DstIt An iterator over HiCR resources
   *
   * Let \f$ S \f$ be a set of producers, and let \f$ D \f$ be a set of
   * consumers. \f$ S, D \f$ must contain at least one element.
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
   * \warning A HiCR::memcpy only reverts to zero-copy communication if the
   *          backend supports it.
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
   */
  size_t capacity() const noexcept;

  /**
   * @returns The number of elements in this consumer channel.
   *
   * Assumes that this channel at the current locality is a consumer.
   *
   * \note We could also define this for producers, and return the number of
   *       un-sent tokens.
   *
   * This is a one-sided blocking call that need not be made collectively.
   *
   * This is a getter function that should complete in \f$ \Theta(1) \f$ time.
   */
  size_t depth() const;

  /**
   * Peeks in the local received queue and returns a pointer to the current
   * token.
   *
   * Assumes that this channel at the current locality is a consumer.
   *
   * @param[out] ptr  A pointer to the current token.
   * @param[out] size The size of the memory region pointed to.
   *
   * @returns <tt>true</tt> if the channel was non-empty.
   * @returns <tt>false</tt> if the channel was empty.
   *
   * This is a one-sided blocking call that need not be made collectively.
   *
   * This is a getter function that should complete in \f$ \Theta(1) \f$ time.
   *
   * \note While this function does not modify the state of the channel, the
   *       contents of the token may be modified by the caller.
   */
  bool peek(void * const ptr, size_t * const size) const;

  /**
   * Removes the current token from the channel, removes it, and moves to the
   * next token (or to an empty channel state).
   *
   * Assumes that this channel at the current locality is a consumer.
   *
   * This is a one-sided blocking call that need not be made collectively.
   */
  void pop();
  
  bool push(const MemorySlot& slot, const size_t offset, const size_t size);

  // TODO register an effect somehow? Need to support two events:
  //   1) full-to-non-full (producer side),
  //   2) empty-to-nonempty (consumer side).

};

