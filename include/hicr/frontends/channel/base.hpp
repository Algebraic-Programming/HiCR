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
 * @file fixedSize/base.hpp
 * @brief Provides base functionality for a fixed-size MPSC channel over HiCR
 * @author S. M Martin
 * @date 14/11/2023
 */

#pragma once

#include <memory>
#include <cstring>
#include <hicr/core/definitions.hpp>
#include <hicr/core/exceptions.hpp>
#include <hicr/core/globalMemorySlot.hpp>
#include <hicr/core/communicationManager.hpp>
#include <hicr/frontends/channel/circularBuffer.hpp>

/**
 * Establishes how many elements are required in the base coordination buffer
 */
enum
{
  _HICR_CHANNEL_COORDINATION_BUFFER_ELEMENT_COUNT = 2
};

/**
 * Establishes how the type of elements required in the base coordination buffer
 */
#define _HICR_CHANNEL_COORDINATION_BUFFER_ELEMENT_TYPE size_t

/**
 * Establishes the value index for the head advance count
 */
enum
{
  _HICR_CHANNEL_HEAD_ADVANCE_COUNT_IDX = 0
};

/**
 * Establishes the value index for the tail advance count
 */
enum
{
  _HICR_CHANNEL_TAIL_ADVANCE_COUNT_IDX = 1
};

namespace HiCR::channel
{

/**
 * Base class definition for a HiCR Multiple-Producer Single-Consumer Channel
 */
class Base
{
  public:

  /**
   * @returns The size of the tokens in this channel.
   *
   * Returns simply the size per token. All tokens have the same size.
   *
   * This is a one-sided blocking call that need not be made collectively.
   *
   * This is a getter function that should complete in \f$ \Theta(1) \f$ time.
   *
   * This function when called on a valid channel instance will never fail.
   */
  [[nodiscard]] __INLINE__ size_t getTokenSize() const noexcept { return _tokenSize; }

  /**
   * This function can be used to check the size of the coordination buffer that needs to be provided
   * in the creation of the producer channel
   *
   * \return Size (bytes) of the coordination buffer
   */
  __INLINE__ static size_t getCoordinationBufferSize() noexcept { return _HICR_CHANNEL_COORDINATION_BUFFER_ELEMENT_COUNT * sizeof(_HICR_CHANNEL_COORDINATION_BUFFER_ELEMENT_TYPE); }

  /**
   * This function can be used to check the size of the coordination buffer that needs to be provided
   * in the creation of the producer channel
   *
   * \param[in] coordinationBuffer Memory slot corresponding to the coordination buffer
   */
  __INLINE__ static void initializeCoordinationBuffer(const std::shared_ptr<LocalMemorySlot> &coordinationBuffer)
  {
    // Checking for correct size
    auto requiredSize = getCoordinationBufferSize();
    auto size         = coordinationBuffer->getSize();
    if (size < requiredSize)
      HICR_THROW_LOGIC("Attempting to initialize coordination buffer size on a memory slot (%lu) smaller than the required size (%lu).\n", size, requiredSize);

    // Getting actual buffer of the coordination buffer
    auto bufferPtr = coordinationBuffer->getPointer();

    // Resetting all its values to zero
    memset(bufferPtr, 0, getCoordinationBufferSize());
  }

  /**
   * This function can be used to check the minimum size of the token buffer that needs to be provided
   * to the consumer channel.
   *
   * \param[in] tokenSize The expected size of the tokens to use in the channel
   * \param[in] capacity The expected capacity (in token count) to use in the channel
   * \return Minimum size (bytes) required of the token buffer
   */
  __INLINE__ static size_t getTokenBufferSize(const size_t tokenSize, const size_t capacity) noexcept { return tokenSize * capacity; }

  /**
   * Returns the current channel depth.
   *
   * If the current channel is a consumer, it corresponds to how many tokens
   * may yet be consumed. If the current channel is a producer, it corresponds
   * the channel capacity minus the returned value equals how many tokens may
   * still be pushed.
   *
   * \note This is not a thread-safe call
   *
   * This is a getter function that should complete in \f$ \Theta(1) \f$ time.
   *
   * @returns The number of tokens in this channel.
   *
   * This function when called on a valid channel instance will never fail.
   */
  [[nodiscard]] __INLINE__ size_t getDepth() const noexcept { return _circularBuffer->getDepth(); }

  /**
   * This function can be used to quickly check whether the channel is full.
   *
   * It affects the internal state of the channel because it detects any updates in the internal state of the buffers
   *
   * \returns true, if the buffer is full
   * \returns false, if the buffer is not full
   */
  [[nodiscard]] __INLINE__ bool isFull() const noexcept { return _circularBuffer->isFull(); }

  /**
   * This function can be used to quickly check whether the channel is empty.
   *
   * It does not affects the internal state of the channel
   *
   * \returns true, if the buffer is empty
   * \returns false, if the buffer is not empty
   */
  [[nodiscard]] __INLINE__ bool isEmpty() const noexcept { return _circularBuffer->isEmpty(); }

  /**
   * A function to obtain the low-level circular buffer where tokens are stored
   * @return The internal circular buffer
   */
  [[nodiscard]] __INLINE__ auto getCircularBuffer() const noexcept { return _circularBuffer.get(); }

  protected:

  /**
   * The constructor of the base Channel class.
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
  Base(CommunicationManager &communicationManager, const std::shared_ptr<LocalMemorySlot> &coordinationBuffer, const size_t tokenSize, const size_t capacity)
    : _communicationManager(&communicationManager),
      _coordinationBuffer(coordinationBuffer),
      _tokenSize(tokenSize)
  {
    if (tokenSize == 0) HICR_THROW_LOGIC("Attempting to create a channel with token size 0.\n");
    if (capacity == 0) HICR_THROW_LOGIC("Attempting to create a channel with zero capacity \n");

    // Checking that the provided coordination buffers have the right size
    auto requiredCoordinationBufferSize = getCoordinationBufferSize();
    auto providedCoordinationBufferSize = _coordinationBuffer->getSize();
    if (providedCoordinationBufferSize < requiredCoordinationBufferSize)
      HICR_THROW_LOGIC("Attempting to create a channel with a local coordination buffer size (%lu) smaller than the required size (%lu).\n",
                       providedCoordinationBufferSize,
                       requiredCoordinationBufferSize);

    // Creating internal circular buffer
    _circularBuffer = std::make_unique<channel::CircularBuffer>(
      capacity,
      &static_cast<_HICR_CHANNEL_COORDINATION_BUFFER_ELEMENT_TYPE *>(coordinationBuffer->getPointer())[_HICR_CHANNEL_HEAD_ADVANCE_COUNT_IDX],
      &static_cast<_HICR_CHANNEL_COORDINATION_BUFFER_ELEMENT_TYPE *>(coordinationBuffer->getPointer())[_HICR_CHANNEL_TAIL_ADVANCE_COUNT_IDX]);
  }

  ~Base() = default;

  /**
   * Get the internal communication buffer assigned to this channel
   * @return The internal communication buffer assigned to this channel
   */
  [[nodiscard]] __INLINE__ CommunicationManager *getCommunicationManager() const { return _communicationManager; }

  /**
   * Get the internal coordination buffer assigned to this channel
   * @return The internal coordination buffer assigned to this channel
   */
  [[nodiscard]] __INLINE__ auto getCoordinationBuffer() const { return _coordinationBuffer; }

  private:

  /**
   * Pointer to the backend that is in charge of executing the memory transfer operations
   */
  CommunicationManager *const _communicationManager;

  /**
   * Local storage of coordination metadata
   */
  const std::shared_ptr<LocalMemorySlot> _coordinationBuffer;
  /**
   * Token size
   */
  const size_t _tokenSize;

  /**
   * Internal channel (logical) circular buffer
   */
  std::unique_ptr<channel::CircularBuffer> _circularBuffer;
};

} // namespace HiCR::channel
