
/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file variableSize/base.hpp
 * @brief extends channel::Base into a base enabling var-size messages
 * @author K. Dichev
 * @date 15/01/2024
 */

#pragma once

#include <memory>
#include <cstring>
#include <hicr/definitions.hpp>
#include <hicr/exceptions.hpp>
#include <hicr/L0/globalMemorySlot.hpp>
#include <hicr/L1/communicationManager.hpp>
#include <frontends/channel/base.hpp>

namespace HiCR
{

namespace channel
{

namespace fixedSize
{

/**
 * Base class definition for a HiCR fixed-size channel
 * Fixed-size version is identical to the base channel.
 */
class Base : public channel::Base
{
  protected:

  /**
   * The constructor of the Channel class.
   *
   * It requires the user to provide the allocated memory slots for the exchange (data) and coordination buffers.
   *
   * \param[in] communicationManager The backend's memory manager to facilitate communication between the producer and consumer sides
   * \param[in] coordinationBuffer This is a small buffer that needs to be allocated at the producer side.
   *            enables the consumer to signal how many tokens it has popped. It may also be used for other coordination signals.
   * \param[in] tokenSize The size of each token.
   * \param[in] capacity The maximum number of tokens that will be held by this channel
   *
   * \internal For this implementation of channels to work correctly, the underlying backend should guarantee that
   * messages (one per token) should arrive in order. That is, if the producer sends tokens 'A' and 'B', the internal
   * counter for messages received for the data buffer should only increase after 'A' was received (even if B arrived
   * before. That is, if the received message counter starts as zero, it will transition to 1 and then to to 2, if
   * 'A' arrives before than 'B', or; directly to 2, if 'B' arrives before 'A'.
   */
  Base(L1::CommunicationManager            &communicationManager,
       std::shared_ptr<L0::LocalMemorySlot> coordinationBuffer,
       const size_t                         tokenSize,
       const size_t                         capacity) : channel::Base(communicationManager, coordinationBuffer, tokenSize, capacity)
  {
  }
};

} // namespace fixedSize

} // namespace channel

} // namespace HiCR
