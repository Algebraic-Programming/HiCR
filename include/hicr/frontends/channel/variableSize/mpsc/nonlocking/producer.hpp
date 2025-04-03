/*
 *   Copyright 2025 Huawei Technologies Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file variableSize/mpsc/nonlocking/producer.hpp
 * @brief Provides variable-sized MPSC producer channel, non-locking version
 * @author K. Dichev
 * @date 10/06/2024
 */

#pragma once

#include <hicr/frontends/channel/variableSize/spsc/producer.hpp>
#include <utility>

namespace HiCR::channel::variableSize::MPSC::nonlocking
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
  Producer(L1::CommunicationManager                   &communicationManager,
           std::shared_ptr<L0::LocalMemorySlot>        sizeInfoBuffer,
           std::shared_ptr<L0::GlobalMemorySlot>       payloadBuffer,
           std::shared_ptr<L0::GlobalMemorySlot>       tokenBuffer,
           const std::shared_ptr<L0::LocalMemorySlot> &internalCoordinationBufferForCounts,
           const std::shared_ptr<L0::LocalMemorySlot> &internalCoordinationBufferForPayloads,
           std::shared_ptr<L0::GlobalMemorySlot>       consumerCoordinationBufferForCounts,
           std::shared_ptr<L0::GlobalMemorySlot>       consumerCoordinationBufferForPayloads,
           const size_t                                payloadCapacity,
           const size_t                                payloadSize,
           const size_t                                capacity)
    : variableSize::SPSC::Producer(communicationManager,
                                   std::move(sizeInfoBuffer),
                                   std::move(payloadBuffer),
                                   std::move(tokenBuffer),
                                   internalCoordinationBufferForCounts,
                                   internalCoordinationBufferForPayloads,
                                   std::move(consumerCoordinationBufferForCounts),
                                   std::move(consumerCoordinationBufferForPayloads),
                                   payloadCapacity,
                                   payloadSize,
                                   capacity)
  {}
  ~Producer() = default;
};

} // namespace HiCR::channel::variableSize::MPSC::nonlocking
