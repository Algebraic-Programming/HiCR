
/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
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
  Producer(L1::CommunicationManager             &communicationManager,
           std::shared_ptr<L0::GlobalMemorySlot> tokenBuffer,
           std::shared_ptr<L0::LocalMemorySlot>  internalCoordinationBuffer,
           std::shared_ptr<L0::GlobalMemorySlot> producerCoordinationBuffer,
           const size_t                          tokenSize,
           const size_t                          capacity)
    : fixedSize::SPSC::Producer(communicationManager, tokenBuffer, internalCoordinationBuffer, producerCoordinationBuffer, tokenSize, capacity)
  {}
  ~Producer() = default;
};

} // namespace nonlocking

} // namespace MPSC

} // namespace fixedSize

} // namespace channel

} // namespace HiCR
