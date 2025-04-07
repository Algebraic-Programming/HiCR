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
 * @file variableSize/base.hpp
 * @brief extends channel::Base into a base enabling var-size messages
 * @author K. Dichev
 * @date 15/01/2024
 */

#pragma once

#include <memory>
#include <cstring>
#include <hicr/core/definitions.hpp>
#include <hicr/core/exceptions.hpp>
#include <hicr/core/globalMemorySlot.hpp>
#include <hicr/core/communicationManager.hpp>
#include <hicr/frontends/channel/base.hpp>

namespace HiCR::channel::variableSize
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
   * \param[in] coordinationBufferForCounts This is a small buffer that enables the consumer to signal how many payloads (as a count) it has popped.
   * \param[in] coordinationBufferForPayloads This is a small buffer that enables the consumer to signal how many bytes from the payload data it has popped.
   * \param[in] capacity The maximum number of elements (possibly different-sized) that can be held by this channel
   * \param[in] payloadCapacity The maximum number of total bytes (including all different sized elements) that will be held by this channel
   * @note: The token size in var-size channels is used only internally, and is passed as having a type size_t (with size sizeof(size_t))
   * The key extension to the base channel class is the use of an extended circular buffer instead of a circular buffer.
   * This is because we need to manage payload head and tail in addition to the head an tail pointers for different elements.
   */
  Base(CommunicationManager                   &communicationManager,
       const std::shared_ptr<LocalMemorySlot> &coordinationBufferForCounts,
       const std::shared_ptr<LocalMemorySlot> &coordinationBufferForPayloads,
       const size_t                            capacity,
       const size_t                            payloadCapacity)
    : channel::Base(communicationManager, coordinationBufferForCounts, sizeof(size_t), capacity),
      _coordinationBufferForCounts(coordinationBufferForCounts),
      _coordinationBufferForPayloads(coordinationBufferForPayloads)
  {
    if (capacity == 0) HICR_THROW_LOGIC("Attempting to create a channel with zero capacity \n");

    // Checking that the provided coordination buffers have the right size
    auto requiredCoordinationBufferSize = 4 * sizeof(_HICR_CHANNEL_COORDINATION_BUFFER_ELEMENT_TYPE);
    auto providedCoordinationBufferSize = coordinationBufferForPayloads->getSize() + coordinationBufferForCounts->getSize();
    if (providedCoordinationBufferSize < requiredCoordinationBufferSize)
      HICR_THROW_LOGIC("Attempting to create a channel with a local coordination buffer size (%lu) smaller than the required size (%lu).\n",
                       providedCoordinationBufferSize,
                       requiredCoordinationBufferSize);

    // Creating internal circular buffer
    _circularBufferForCounts = std::make_unique<channel::CircularBuffer>(
      capacity,
      &static_cast<_HICR_CHANNEL_COORDINATION_BUFFER_ELEMENT_TYPE *>(coordinationBufferForCounts->getPointer())[_HICR_CHANNEL_HEAD_ADVANCE_COUNT_IDX],
      &static_cast<_HICR_CHANNEL_COORDINATION_BUFFER_ELEMENT_TYPE *>(coordinationBufferForCounts->getPointer())[_HICR_CHANNEL_TAIL_ADVANCE_COUNT_IDX]);

    _circularBufferForPayloads = std::make_unique<channel::CircularBuffer>(
      payloadCapacity,
      &static_cast<_HICR_CHANNEL_COORDINATION_BUFFER_ELEMENT_TYPE *>(coordinationBufferForPayloads->getPointer())[_HICR_CHANNEL_HEAD_ADVANCE_COUNT_IDX],
      &static_cast<_HICR_CHANNEL_COORDINATION_BUFFER_ELEMENT_TYPE *>(coordinationBufferForPayloads->getPointer())[_HICR_CHANNEL_TAIL_ADVANCE_COUNT_IDX]);
  }

  /**
   * Gets the internal circular buffer used for token counts
   * @return the internal circular buffer used for token counts
   */
  [[nodiscard]] __INLINE__ auto getCircularBufferForCounts() const { return _circularBufferForCounts.get(); }

  /**
   * Gets the internal circular buffer used for token payloads
   * @return the internal circular buffer used for token payloads
   */
  [[nodiscard]] __INLINE__ auto getCircularBufferForPayloads() const { return _circularBufferForPayloads.get(); }

  /**
   * Gets the internal coordination buffer used for token counts
   * @return the internal coordination buffer used for token counts
   */
  [[nodiscard]] __INLINE__ auto getCoordinationBufferForCounts() const { return _coordinationBufferForCounts; }

  /**
   * Gets the internal coordination buffer used for token payloads
   * @return the internal coordination buffer used for token payloads
   */
  [[nodiscard]] __INLINE__ auto getCoordinationBufferForPayloads() const { return _coordinationBufferForPayloads; }

  private:

  /**
   * Circular buffer holding size counts (head/tail)
   */
  std::unique_ptr<channel::CircularBuffer> _circularBufferForCounts;
  /**
   * Circular buffer holding payload (head/tail)
   */
  std::unique_ptr<channel::CircularBuffer> _circularBufferForPayloads;
  /**
   * Pointer to Local slot associated with circular buffer for counts
   */
  const std::shared_ptr<LocalMemorySlot> _coordinationBufferForCounts;
  /**
   * Pointer to Local slot associated with circular buffer for payloads
   */
  const std::shared_ptr<LocalMemorySlot> _coordinationBufferForPayloads;
};

} // namespace HiCR::channel::variableSize
