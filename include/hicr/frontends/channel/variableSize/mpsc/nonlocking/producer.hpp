
/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file variableSize/mpsc/nonlocking/producer.hpp
 * @brief Provides variable-sized MPSC producer channel, non-locking version
 * @author K. Dichev
 * @date 10/06/2024
 */

#pragma once

#include <hicr/frontends/channel/variableSize/spsc/producer.hpp>

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
 * Class definition for a variable-sized non-locking MPSC producer channel
 *
 * It is identical to the variable-sized SPSC producer Channel
 *
 */
class Producer final : public variableSize::SPSC::Producer
{
  public:

  /**
   * The constructor of the variable-sized producer channel class.
   *
   * It requires the user to provide the allocated memory slots for the exchange (data) and coordination buffers.
   *
   * \param[in] communicationManager The backend to facilitate communication between the producer and consumer sides
   * \param[in] sizeInfoBuffer The local memory slot used to hold the information about the next message size
   * \param[in] payloadBuffer The global memory slot pertaining to the payload of all messages. The producer will push messages into this
   *            buffer, while there is enough space. This buffer should be large enough to hold at least the largest of the variable-size messages.
   * \param[in] tokenBuffer The memory slot pertaining to the token buffer, which is used to hold message size data.
   *            The producer will push message sizes into this buffer, while there is enough space. This buffer should be large enough to
   *            hold at least one message size.
   * \param[in] internalCoordinationBufferForCounts This is a small buffer to hold the internal (local) state of the channel's message counts
   * \param[in] internalCoordinationBufferForPayloads This is a small buffer to hold the internal (local) state of the channel's payload sizes (in bytes)
   * \param[in] consumerCoordinationBufferForCounts A global reference of the consumer's own coordination buffer to check for updates on message counts
   * \param[in] consumerCoordinationBufferForPayloads A global reference of the consumer's own coordination buffer to check for updates on payload sizes (in bytes)
   * \param[in] payloadCapacity capacity in bytes of the buffer for message payloads
   * \param[in] payloadSize size in bytes of the datatype used for variable-sized messages
   * \param[in] capacity The maximum number of tokens that will be held by this channel
   */
  Producer(L1::CommunicationManager             &communicationManager,
           std::shared_ptr<L0::LocalMemorySlot>  sizeInfoBuffer,
           std::shared_ptr<L0::GlobalMemorySlot> payloadBuffer,
           std::shared_ptr<L0::GlobalMemorySlot> tokenBuffer,
           std::shared_ptr<L0::LocalMemorySlot>  internalCoordinationBufferForCounts,
           std::shared_ptr<L0::LocalMemorySlot>  internalCoordinationBufferForPayloads,
           std::shared_ptr<L0::GlobalMemorySlot> consumerCoordinationBufferForCounts,
           std::shared_ptr<L0::GlobalMemorySlot> consumerCoordinationBufferForPayloads,
           const size_t                          payloadCapacity,
           const size_t                          payloadSize,
           const size_t                          capacity)
    : variableSize::SPSC::Producer(communicationManager,
                                   sizeInfoBuffer,
                                   payloadBuffer,
                                   tokenBuffer,
                                   internalCoordinationBufferForCounts,
                                   internalCoordinationBufferForPayloads,
                                   consumerCoordinationBufferForCounts,
                                   consumerCoordinationBufferForPayloads,
                                   payloadCapacity,
                                   payloadSize,
                                   capacity)
  {}
  ~Producer() = default;
};

} // namespace nonlocking

} // namespace MPSC

} // namespace variableSize

} // namespace channel

} // namespace HiCR
