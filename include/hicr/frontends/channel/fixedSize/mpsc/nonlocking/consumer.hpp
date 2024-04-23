
/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file fixedSize/mpsc/nonlocking/consumer.hpp
 * @brief Provides Consumer functionality for MPSC based on SPSC, that is without global locks
 * @author K. Dichev
 * @date 08/04/2024
 */

#pragma once

#include <queue>
#include <hicr/core/definitions.hpp>
#include <hicr/core/exceptions.hpp>
#include <hicr/frontends/channel/fixedSize/base.hpp>
#include <hicr/frontends/channel/fixedSize/spsc/consumer.hpp>

namespace HiCR
{

namespace channel
{

namespace fixedSize
{

namespace MPSC
{

namespace nonlocking
{

/**
 * Class definition for a non-locking Consumer MPSC Channel
 *
 * It exposes the functionality based on SPSC channels, and without global locks
 *
 */
class Consumer
{
  public:

  /**
   * The constructor of the consumer channel class.
   *
   * It requires the user to provide the allocated memory slots for the exchange (data) and coordination buffers.
   *
   * \param[in] communicationManager The backend to facilitate communication between the producer and consumer sides
   * \param[in] tokenBuffers The list of p memory slots pertaining for the p producers. Each producer will push new
   * tokens into its own buffer, relying on an SPSC channel
   * token.
   * \param[in] internalCoordinationBuffers The list of p coordination buffers on the consumer side, each of them 
   * responsible for the consumer side of 1 of p SPSC channels
   * \param[in] producerCoordinationBuffers The list of p coordination buffers on the producer side, each of them 
   * responsible for the producer side of 1 of p SPSC channels
   * \param[in] tokenSize The size of each token.
   * \param[in] capacity The maximum number of tokens that will be held by this channel
   */
  Consumer(L1::CommunicationManager                          &communicationManager,
           std::vector<std::shared_ptr<L0::GlobalMemorySlot>> tokenBuffers,
           std::vector<std::shared_ptr<L0::LocalMemorySlot>>  internalCoordinationBuffers,
           std::vector<std::shared_ptr<L0::GlobalMemorySlot>> producerCoordinationBuffers,
           const size_t                                       tokenSize,
           const size_t                                       capacity)
    : _communicationManager(&communicationManager)
  {
    // make sure producer and consumer sides provide p elements, equalling
    // the number of producers
    assert(internalCoordinationBuffers.size() == producerCoordinationBuffers.size());
    assert(internalCoordinationBuffers.size() == tokenBuffers.size());
    // create p (= number of producers) SPSC channels
    for (size_t i = 0; i < internalCoordinationBuffers.size(); i++)
    {
      std::shared_ptr<fixedSize::SPSC::Consumer> consumerPtr(
        new fixedSize::SPSC::Consumer(communicationManager, tokenBuffers[i], internalCoordinationBuffers[i], producerCoordinationBuffers[i], tokenSize, capacity));
      _spscList.push_back(consumerPtr);
      _depths.push_back(0);
    }
  }

  ~Consumer() = default;

  /**
   * Peeks in the local received queue and returns a pair of elements
   * (channel-id, position)
   *
   * This is a one-sided blocking call that need not be made collectively.
   *
   * @param[in] pos The token position required. pos = 0 indicates earliest token that
   *                is currently present in the buffer. pos = getDepth()-1 indicates
   *                the latest token to have arrived.
   *
   * @returns A pair of elements (ID, position), where ID is the index of 
   * an SPSC channel (0 <= index< \# producers), and position is the position in this channel.
   */
  __INLINE__ std::array<size_t, 2> peek(const size_t pos = 0)
  {
    std::array<size_t, 2> ret = {0};
    // @ToDo: to support pos > 0, we need to modify _channelPushes to
    // be of type std::vector instead of std::queue
    if (pos > 0) HICR_THROW_LOGIC("Nonblocking MPSC not yet implemented for peek with n!=0");

    _communicationManager->flushReceived();
    updateDepth();
    if (_channelPushes.empty()) HICR_THROW_RUNTIME("Attempting to peek position (%lu) but supporting queue has size (%lu)", pos, _channelPushes.size());

    size_t channelId = _channelPushes.front(); // front() returns the first (i.e. oldest) element
    if (channelId >= _spscList.size()) { HICR_THROW_LOGIC("channelId (%lu) >= _spscList.size() (%lu)", channelId, _spscList.size()); }
    ret[0] = channelId;
    ret[1] = _spscList[channelId]->peek();

    return ret;
  }

  /**
   * Sums up the depths of all SPSC channels
   * This depth is likely to be out-of-date, call updateDepth() for a (more)
   * recent depth
   * @returns total sum of SPSC channel depths after latest updateDepth() call
   */
  __INLINE__ size_t getDepth()
  {
    size_t totalDepth = 0;
    for (auto d : _depths) { totalDepth += d; }
    //    if (totalDepth != _channelPushes.size()) {
    //      HICR_THROW_LOGIC("Helper FIFO and channels are out of sync, implemenation issue! getDepth (%lu) != _channelPushes.size() (%lu)", totalDepth, _channelPushes.size());
    //    }
    return totalDepth;
  }

  /**
   * Returns if all SPSC channels of this MPSC are empty
   * @returns True if all SPSC channels are empty
   */
  __INLINE__ bool isEmpty() { return (getDepth() == 0); }

  /**
   * pop removes n elements from the MPSC channel. These
   * elements may well be removed from across multiple SPSC channels.
   * @param[in] n How many tokens to pop. Optional; default is one.
   */
  __INLINE__ void pop(const size_t n = 1)
  {
    updateDepth();
    // If the exchange buffer does not have enough tokens, reject operation
    if (n > getDepth())
      HICR_THROW_RUNTIME("Attempting to pop (%lu) tokens, which is more than the number of current tokens in the channel (%lu)", n, getDepth());
    else
    {
      size_t channelFirstPushed = _channelPushes.front();
      if (channelFirstPushed >= _spscList.size())
        HICR_THROW_LOGIC("Index of latest push channel incorrect!");
      else
      {
        // pop n elements from the SPSCs in the order recorded in the helper
        // FIFO _channelPushes, and also update the FIFO itself
        for (size_t i = 0; i < n; i++)
        {
          _spscList[channelFirstPushed]->pop();
          _depths[channelFirstPushed]--;
          _channelPushes.pop();
        }
      }
    }
  }

  /**
  * Update the depth of all SPSC channels. If increase in the depth
  * of any channel is detected, record this in the helper FIFO _channelPushes.
  * If entries were pushed in multiple channels, we do not look for the 
  * chronologically correct order they were inserted in. Instead, we 
  * choose an arbitrary order (first SPSC channels with updates gets
  * first/oldest entry in FIFO, last SPSC channel gets last/newest entry in FIFO.
  */
  __INLINE__ void updateDepth()
  {
    std::vector<size_t> newDepths(_spscList.size());
    // Note that after calling updateDepth() on each SPSC channel,
    // we must accept this state as a new temporary snapshot in newDepths
    // It is possible that during our iterating through newDepths, producers have
    // sent more elements already, which will be handled in later
    // updateDepth calls
    for (size_t i = 0; i < _spscList.size(); i++)
    {
      _spscList[i]->updateDepth();
      newDepths[i] = _spscList[i]->getDepth();
    }

    for (size_t i = 0; i < _spscList.size(); i++)
    {
      for (size_t j = _depths[i]; j < newDepths[i]; j++) { _channelPushes.push(i); }
    }
    std::swap(_depths, newDepths);
    if (getDepth() != _channelPushes.size()) { HICR_THROW_LOGIC("getDepth (%lu) != _channelPushes.size() (%lu)", getDepth(), _channelPushes.size()); }
  }

  private:

  /**
   * List of SPSC channels this MPSC consists of
   */
  std::vector<std::shared_ptr<channel::fixedSize::SPSC::Consumer>> _spscList;
  /**
   * A FIFO used to record in which SPSC channel elements were pushed, and (roughly) in what order
   */
  std::queue<size_t> _channelPushes;

  /**
   * A snapshot of the last recorded depths in all SPSC channels (initialized with 0s)
   */
  std::vector<size_t> _depths;
  /**
   * Pointer to the backend that is in charge of executing the memory transfer operations
   */
  L1::CommunicationManager *const _communicationManager;
};

} // namespace nonlocking

} // namespace MPSC

} // namespace fixedSize

} // namespace channel

} // namespace HiCR
