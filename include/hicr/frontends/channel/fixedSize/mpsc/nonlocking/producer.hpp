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
 * @file fixedSize/mpsc/nonlocking/producer.hpp
 * @brief Provides prod
 * @author K. Dichev
 * @date 08/04/2024
 */

#pragma once

#include <hicr/core/definitions.hpp>
#include <hicr/core/exceptions.hpp>
#include <hicr/frontends/channel/fixedSize/base.hpp>
#include <hicr/frontends/channel/fixedSize/spsc/producer.hpp>
#include <utility>

namespace HiCR::channel::fixedSize::MPSC::nonlocking
{

/**
 * Class definition for a non-locking Producer MPSC Channel
 * It is entirely identical to the Producer SPSC Channel
 *
 */
class Producer final : public fixedSize::SPSC::Producer
{
  public:

  /**
   * This constructor simply calls the SPSC Producer constructor
   *
   * \param[in] communicationManager The backend to facilitate communication between the producer and consumer sides
   * \param[in] tokenBuffer The memory slot pertaining to the token buffer. The producer will push new
   *            tokens into this buffer, while there is enough space. This buffer should be big enough to hold at least one token.
   * \param[in] internalCoordinationBuffer This is a small buffer to hold the internal (loca) state of the channel's circular buffer
   * \param[in] producerCoordinationBuffer A global reference of the producer's own coordination buffer to check for pop updates produced by the remote consumer
   * \param[in] tokenSize The size of each token.
   * \param[in] capacity The maximum number of tokens that will be held by this channel
   */
  Producer(CommunicationManager                    &communicationManager,
           std::shared_ptr<GlobalMemorySlot>        tokenBuffer,
           const std::shared_ptr<LocalMemorySlot>  &internalCoordinationBuffer,
           const std::shared_ptr<GlobalMemorySlot> &producerCoordinationBuffer,
           const size_t                             tokenSize,
           const size_t                             capacity)
    : fixedSize::SPSC::Producer(communicationManager, std::move(tokenBuffer), internalCoordinationBuffer, producerCoordinationBuffer, tokenSize, capacity)
  {}
  ~Producer() = default;
};

} // namespace HiCR::channel::fixedSize::MPSC::nonlocking
