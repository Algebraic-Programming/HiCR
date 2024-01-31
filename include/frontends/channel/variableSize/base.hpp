
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

namespace variableSize
{

/**
 * A HiCR variable-size channel base
 */
class Base : public channel::Base
{
  protected:

  /**
   * The constructor of the extended base Channel class.
   *
   * It requires the user to provide the allocated memory slots for the exchange (data) and coordination buffers.
   *
   * \param[in] communicationManager The backend's memory manager to facilitate communication between the producer and consumer sides
   * \param[in] coordinationBufferForCounts This is a small buffer that needs to be allocated at the producer side. It
   *            enables the consumer to signal how many payloads (as a count) it has popped.
   * \param[in] coordinationBufferForPayloads This is a small buffer that needs to be allocated at the producer side. It
   *            enables the consumer to signal how many bytes from the payload data it has popped.
   * \param[in] capacity The maximum number of elements (possibly different-sized) that can be held by this channel
   * \param[in] payloadCapacity The maximum number of total bytes (including all different sized elements) that will be held by this channel
   * @note: The token size in var-size channels is used only internally, and is passed as having a type size_t (with size sizeof(size_t))
   * The key extension to the base channel class is the use of an extended circular buffer instead of a circular buffer.
   * This is because we need to manage payload head and tail in addition to the head an tail pointers for different elements.
   */
  Base(L1::CommunicationManager &communicationManager,
       std::shared_ptr<L0::LocalMemorySlot> coordinationBufferForCounts,
       std::shared_ptr<L0::LocalMemorySlot> coordinationBufferForPayloads,
       const size_t capacity,
       const size_t payloadCapacity) : channel::Base(communicationManager, coordinationBufferForCounts, sizeof(size_t), capacity),
                                       _coordinationBufferForPayloads(coordinationBufferForPayloads)
  {
    if (capacity == 0) HICR_THROW_LOGIC("Attempting to create a channel with zero capacity \n");

    // Checking that the provided coordination buffers have the right size
    auto requiredCoordinationBufferSize = 4 * sizeof(_HICR_CHANNEL_COORDINATION_BUFFER_ELEMENT_TYPE);
    auto providedCoordinationBufferSize = coordinationBufferForPayloads->getSize() + coordinationBufferForCounts->getSize();
    if (providedCoordinationBufferSize < requiredCoordinationBufferSize) HICR_THROW_LOGIC("Attempting to create a channel with a local coordination buffer size (%lu) smaller than the required size (%lu).\n", providedCoordinationBufferSize, requiredCoordinationBufferSize);

    // Creating internal circular buffer
    _circularBuffer = std::make_unique<channel::CircularBuffer>(capacity,
                                                                (((_HICR_CHANNEL_COORDINATION_BUFFER_ELEMENT_TYPE *)coordinationBufferForCounts->getPointer()) + _HICR_CHANNEL_HEAD_ADVANCE_COUNT_IDX),
                                                                (((_HICR_CHANNEL_COORDINATION_BUFFER_ELEMENT_TYPE *)coordinationBufferForCounts->getPointer()) + _HICR_CHANNEL_TAIL_ADVANCE_COUNT_IDX));

    _circularBufferForPayloads = std::make_unique<channel::CircularBuffer>(payloadCapacity,
                                                                           (((_HICR_CHANNEL_COORDINATION_BUFFER_ELEMENT_TYPE *)coordinationBufferForPayloads->getPointer()) + _HICR_CHANNEL_HEAD_ADVANCE_COUNT_IDX),
                                                                           (((_HICR_CHANNEL_COORDINATION_BUFFER_ELEMENT_TYPE *)coordinationBufferForPayloads->getPointer()) + _HICR_CHANNEL_TAIL_ADVANCE_COUNT_IDX));
  }

  // virtual ~Base() = default;

  protected:

  /**
   * Circular buffer holding payload (head/tail)
   */
  std::unique_ptr<channel::CircularBuffer> _circularBufferForPayloads;
  /**
   * Local storage of coordination metadata relating to payload head/tail
   */
  const std::shared_ptr<L0::LocalMemorySlot> _coordinationBufferForPayloads;
};

} // namespace variableSize

} // namespace channel

} // namespace HiCR
