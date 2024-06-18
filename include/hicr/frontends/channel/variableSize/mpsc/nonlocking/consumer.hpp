
/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file variableSize/mpsc/locking/consumer.hpp
 * @brief Provides variable-sized MPSC consumer channel, non-locking version
 * @author K. Dichev
 * @date 10/06/2024
 */

#pragma once

#include <queue>
#include <hicr/frontends/channel/variableSize/spsc/consumer.hpp>

namespace HiCR
{

namespace channel
{

namespace variableSize
{

namespace MPSC
{

namespace nonlocking
{

/**
 * Class definition for a variable-sized non-locking MPSC consumer channel
 *
 */
class Consumer
{
  public:

  /**
   * The constructor of the variable-sized consumer channel class.
   *
   * It requires the user to provide the allocated memory slots for the exchange (data) and coordination buffers.
   *
   * \param[in] communicationManager The backend to facilitate communication between the producer and consumer sides
   * \param[in] payloadBuffers The list of memory slots pertaining to the payload buffers. The producers will push new messages
   *            into these buffers as long as space allows. This buffer should be large enough to hold at least the
   *            largest message of the variable-sized messages to be pushed.
   * \param[in] tokenBuffers The list of memory slot pertaining to the token buffers. They are only used to exchange internal metadata
   *            about the sizes of the individual messages being sent.
   * \param[in] internalCoordinationBufferForCounts This is a small buffer to hold the internal (local) state of the
   *            channel's circular buffer message counts
   * \param[in] internalCoordinationBufferForPayloads This is a small buffer to hold the internal (local) state of the
   *            channel's circular buffer payload sizes (in bytes)
   * \param[in] producerCoordinationBufferForCounts A global reference to the producer channel's internal coordination
   *            buffer for message counts, used for remote updates on pop()
   * \param[in] producerCoordinationBufferForPayloads A global reference to the producer channel's internal coordination
   *            buffer for payload sizes (in bytes), used for remote updates on pop()
   * \param[in] payloadCapacity The capacity (in bytes) of the buffer for variable-sized messages
   * \param[in] payloadSize The size of the payload datatype used to hold variable-sized messages of this datatype in the channel
   * \param[in] capacity The maximum number of tokens that will be held by this channel
   * @note: The token size in var-size channels is used only internally, and is passed as having a type size_t (with size sizeof(size_t))
   */
  Consumer(L1::CommunicationManager                          &communicationManager,
           std::vector<std::shared_ptr<L0::GlobalMemorySlot>> payloadBuffers,
           std::vector<std::shared_ptr<L0::GlobalMemorySlot>> tokenBuffers,
           std::vector<std::shared_ptr<L0::LocalMemorySlot>>  internalCoordinationBufferForCounts,
           std::vector<std::shared_ptr<L0::LocalMemorySlot>>  internalCoordinationBufferForPayloads,
           std::vector<std::shared_ptr<L0::GlobalMemorySlot>> producerCoordinationBufferForCounts,
           std::vector<std::shared_ptr<L0::GlobalMemorySlot>> producerCoordinationBufferForPayloads,
           const size_t                                       payloadCapacity,
           const size_t                                       payloadSize,
           const size_t                                       capacity)
    : _communicationManager(&communicationManager)
  {
    // make sure producer and consumer sides have the same element size
    // the size is hopefully the producer count
    assert(!internalCoordinationBufferForCounts.empty());
    auto producerCount = internalCoordinationBufferForCounts.size();
    assert(producerCount == internalCoordinationBufferForPayloads.size());
    assert(producerCount == producerCoordinationBufferForCounts.size());
    assert(producerCount == producerCoordinationBufferForPayloads.size());

    // create p (= number of producers) SPSC channels
    for (size_t i = 0; i < producerCount; i++)
    {
      std::shared_ptr<variableSize::SPSC::Consumer> consumerPtr(new variableSize::SPSC::Consumer(communicationManager,
                                                                                                 payloadBuffers[i],
                                                                                                 tokenBuffers[i],
                                                                                                 internalCoordinationBufferForCounts[i],
                                                                                                 internalCoordinationBufferForPayloads[i],
                                                                                                 producerCoordinationBufferForCounts[i],
                                                                                                 producerCoordinationBufferForPayloads[i],
                                                                                                 payloadCapacity,
                                                                                                 payloadSize,
                                                                                                 capacity));
      _spscList.push_back(consumerPtr);

      /*
      * Note that it is important to record messages that might already have been received
      * immediately upon creation of the SPSC channel. Therefore we do not reset
      * _depths to zero, and check for "early" received messages
      */
      _depths.push_back(consumerPtr->getDepth());
      for (size_t j = 0; j < _depths.back(); j++) { _channelPushes.push(i); }
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
   * @returns Three elements (ID, position, length), where:
   * - ID is the index of an SPSC channel (0 <= index< \# producers)
   * - position is the position of the payload in channel ID
   * - length is the length in bytes of the payload in channel ID
   */
  __INLINE__ std::array<size_t, 3> peek(const size_t pos = 0)
  {
    std::array<size_t, 3> ret = {0};
    // @ToDo: to support pos > 0, we need to modify _channelPushes to
    // be of type std::vector instead of std::queue
    if (pos > 0) HICR_THROW_LOGIC("Nonblocking MPSC not yet implemented for peek with n!=0");

    _communicationManager->flushReceived();
    updateDepth();
    if (_channelPushes.empty()) HICR_THROW_RUNTIME("Attempting to peek position (%lu) but supporting queue has size (%lu)", pos, _channelPushes.size());

    size_t channelId = _channelPushes.front(); // front() returns the first (i.e. oldest) element
    if (channelId >= _spscList.size()) { HICR_THROW_LOGIC("channelId (%lu) >= _spscList.size() (%lu)", channelId, _spscList.size()); }
    ret[0] = channelId;
    ret[1] = _spscList[channelId]->peek()[0];
    ret[2] = _spscList[channelId]->peek()[1];

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

    if (totalDepth != _channelPushes.size())
    {
      HICR_THROW_LOGIC("Helper FIFO and channels are out of sync, implemenation issue! getDepth (%lu) != _channelPushes.size() (%lu)", totalDepth, _channelPushes.size());
    }
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
    /*
     * Note that after calling updateDepth() on each SPSC channel,
     * we must accept this state as a new temporary snapshot in newDepths.
     * It is possible that during our iterating through newDepths, producers have
     * sent more elements already, which will be handled in later updateDepth calls.
     */

    for (size_t i = 0; i < _spscList.size(); i++)
    {
      _spscList[i]->updateDepth();
      newDepths[i] = _spscList[i]->getDepth();
    }

    for (size_t i = 0; i < _spscList.size(); i++)
    {
      assert(_depths[i] <= newDepths[i]);
      for (size_t j = _depths[i]; j < newDepths[i]; j++) { _channelPushes.push(i); }
    }
    std::swap(_depths, newDepths);
    if (getDepth() != _channelPushes.size()) { HICR_THROW_LOGIC("getDepth (%lu) != _channelPushes.size() (%lu)", getDepth(), _channelPushes.size()); }
  }

  private:

  /**
   * List of SPSC channels this MPSC consists of
   */
  std::vector<std::shared_ptr<channel::variableSize::SPSC::Consumer>> _spscList;
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

} // namespace variableSize

} // namespace channel

} // namespace HiCR
