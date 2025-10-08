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
 * @file variableSize/spsc/producer.hpp
 * @brief Provides functionality for a var-size SPSC producer channel
 * @author K. Dichev & A. N. Yzelman & S. M Martin
 * @date 15/01/2024
 */

#pragma once

#include <hicr/frontends/channel/variableSize/base.hpp>
#include <utility>

namespace HiCR::channel::variableSize::SPSC
{

/**
 * Class definition for a HiCR Producer Channel
 *
 * It exposes the functionality to be expected for a producer channel
 *
 */
class Producer : public variableSize::Base
{
  public:

  /**
   * The constructor of the variable-sized producer channel class.
   *
   * It requires the user to provide the allocated memory slots for the exchange (data) and coordination buffers.
   *
   * \param[in] coordinationCommunicationManager The backend's memory manager to facilitate communication between the producer and consumer coordination buffers
   * \param[in] payloadCommunicationManager The backend's memory manager to facilitate communication between the producer and consumer payload buffers
   * \param[in] sizeInfoBuffer The local memory slot used to hold the information about the next message size
   * \param[in] payloadBuffer The global memory slot pertaining to the payload of all messages. The producer will push messages into this
   *            buffer, while there is enough space. This buffer should be large enough to hold twice the capacity specified by \ref payloadCapacity argument.
   *            Half of the buffer is used as excess buffer to avoid internal fragmentation of messages
   * \param[in] tokenBuffer The memory slot pertaining to the token buffer, which is used to hold message size data.
   *            The producer will push message sizes into this buffer, while there is enough space. This buffer should be large enough to
   *            hold at least one message size.
   * \param[in] internalCoordinationBufferForCounts This is a small buffer to hold the internal (local) state of the channel's message counts
   * \param[in] internalCoordinationBufferForPayloads This is a small buffer to hold the internal (local) state of the channel's payload sizes (in bytes)
   * \param[in] consumerCoordinationBufferForCounts A global reference of the consumer's own coordination buffer to check for updates on message counts
   * \param[in] consumerCoordinationBufferForPayloads A global reference of the consumer's own coordination buffer to check for updates on payload sizes (in bytes)
   * \param[in] payloadCapacity capacity in bytes of the buffer for message payloads.
   * \param[in] payloadSize size in bytes of the datatype used for variable-sized messages. 
   * \param[in] capacity The maximum number of tokens that will be held by this channel
   */
  Producer(CommunicationManager                   &coordinationCommunicationManager,
           CommunicationManager                   &payloadCommunicationManager,
           std::shared_ptr<LocalMemorySlot>        sizeInfoBuffer,
           std::shared_ptr<GlobalMemorySlot>       payloadBuffer,
           std::shared_ptr<GlobalMemorySlot>       tokenBuffer,
           const std::shared_ptr<LocalMemorySlot> &internalCoordinationBufferForCounts,
           const std::shared_ptr<LocalMemorySlot> &internalCoordinationBufferForPayloads,
           std::shared_ptr<GlobalMemorySlot>       consumerCoordinationBufferForCounts,
           std::shared_ptr<GlobalMemorySlot>       consumerCoordinationBufferForPayloads,
           const size_t                            payloadCapacity,
           const size_t                            payloadSize,
           const size_t                            capacity)
    : variableSize::Base(coordinationCommunicationManager,
                         payloadCommunicationManager,
                         internalCoordinationBufferForCounts,
                         internalCoordinationBufferForPayloads,
                         capacity,
                         payloadCapacity),
      _payloadBuffer(std::move(payloadBuffer)),
      _sizeInfoBuffer(std::move(sizeInfoBuffer)),
      _payloadSize(payloadSize),
      _tokenBuffer(std::move(tokenBuffer)),
      _consumerCoordinationBufferForCounts(std::move(consumerCoordinationBufferForCounts)),
      _consumerCoordinationBufferForPayloads(std::move(consumerCoordinationBufferForPayloads))
  {}

  ~Producer() = default;

  /**
   * Puts new variable-sized messages unto the channel. 
   * The implementation consists of two phases. In phase 1, we copy the
   * payload data. In phase 2, we copy the message size of the data we
   * just copied. The reason we postpone copying the data size is following:
   * The method getDepth is now simply a check for the depth of the sizes buffer,
   * since we guarantee that the copying of the actual payload has already
   * happened (phase 1 before phase 2).
   *
   * This is a one-sided blocking primitive that need not be made collectively.
   *
   * @param[in] sourceSlot  Source slot (buffer) to copy into buffer
   * @param[in] n  Number of times to copy this \p sourceSlot into buffer
   *
   * This operation will fail with an exception if:
   *  - The source buffer is smaller than required
   *  - The operation would exceed the buffer size
   *
   * A call to this function throws an exception if:
   *  -# the channel at this locality is a consumer.
   *
   * \internal This variant could be expressed as a call to the next one.
   */
  __INLINE__ void push(const std::shared_ptr<LocalMemorySlot> &sourceSlot, const size_t n = 1)
  {
    if (n != 1) HICR_THROW_RUNTIME("HiCR currently has no implementation for n != 1 with push(sourceSlot, n) for variable size version.");

    // Updating depth of token (message sizes) and payload buffers
    updateDepth();

    /**
     * Payload copy.
     * 
     * We partition the payload buffer in 2 parts:
     * - payload buffer: it is the logical size of the buffer, and the channel will work
     *                   as if a buffer of that size was passed as input
     * - excess buffer: extra space needed for the example described below. This can not 
     *                  be directly used from the outside, but is used by the channel
     *                  to avoid fragmentation: we currently require the token to be pushed
     *                  and peeked as a contiguous memory region
     * 
     * 2 possible scenarios after the push:
     * 
     * 1) we have space between the head and the end of the buffer
     * 2) we do not have enough space ahead after the push, but we have it at the beginning of the buffer
     *    and their sum (i.e., channel capacity - channel depth) allows us to push the token. 
     * 
     * If 2), we push to the excess buffer to avoid breaking the token in 2 chunks. 
     * The head is always pointing to a valid position in the buffer, meaning that when a push into the
     * excess buffer is made, the logical position of the head behaves as if there were no excess buffer (See below). 
     * An excess buffer with the same size as the payload buffer guarantees that all kind of
     * token size will succeed. 
     * 
     * 0               buffer capacity                End of excess buffer
     *                 TAIL                 HEAD1
     * |-------|--------|-------|-------------|----------|          In this case HEAD1 indicates where the the token
     *        HEAD                                                  ends in the implementation, while HEAD is its logical position
     *                                                              and observable value
     * */

    // Get bytes required to push the token
    size_t requiredPayloadBufferSize = sourceSlot->getSize();

    // Throw exception if the token can not be pushed
    if (isFull(requiredPayloadBufferSize) == true)
    {
      HICR_THROW_RUNTIME("Attempting to push a token while the channel is full.\nChannel depth: %lu capacity: %lu\nPayload depth: %lu capacity: %lu",
                         getCircularBufferForCounts()->getDepth(),
                         getCircularBufferForCounts()->getCapacity(),
                         getCircularBufferForPayloads()->getDepth(),
                         getCircularBufferForPayloads()->getCapacity());
    }

    // Get communication managers
    auto payloadCommunicationManager      = getPayloadCommunicationManager();
    auto coordinationCommunicationManager = getCoordinationCommunicationManager();

    //  Payload copy. Just push to the channel, we know there is enough space
    payloadCommunicationManager->memcpy(_payloadBuffer, getPayloadHeadPosition(), sourceSlot, 0, requiredPayloadBufferSize);
    payloadCommunicationManager->fence(sourceSlot, 1, 0);

    // Advance the head in the payload buffer
    getCircularBufferForPayloads()->advanceHead(requiredPayloadBufferSize);

    // update the consumer coordination buffers (consumer does not update its own coordination head positions)
    coordinationCommunicationManager->memcpy(_consumerCoordinationBufferForPayloads,
                                             _HICR_CHANNEL_HEAD_ADVANCE_COUNT_IDX * sizeof(size_t),
                                             getCoordinationBufferForPayloads(),
                                             _HICR_CHANNEL_HEAD_ADVANCE_COUNT_IDX * sizeof(size_t),
                                             sizeof(size_t));
    coordinationCommunicationManager->fence(getCoordinationBufferForPayloads(), 1, 0);

    /*
     * Part 2: Copy the message size
     */
    auto *sizeInfoBufferPtr = static_cast<size_t *>(_sizeInfoBuffer->getPointer());
    sizeInfoBufferPtr[0]    = requiredPayloadBufferSize;

    coordinationCommunicationManager->memcpy(_tokenBuffer,                                                     /* destination */
                                             getTokenSize() * getCircularBufferForCounts()->getHeadPosition(), /* dst_offset */
                                             _sizeInfoBuffer,                                                  /* source */
                                             0,                                                                /* src_offset */
                                             getTokenSize());                                                  /* size */
    coordinationCommunicationManager->fence(_sizeInfoBuffer, 1, 0);
    getCircularBufferForCounts()->advanceHead(1);

    coordinationCommunicationManager->memcpy(_consumerCoordinationBufferForCounts,
                                             _HICR_CHANNEL_HEAD_ADVANCE_COUNT_IDX * sizeof(size_t),
                                             getCoordinationBufferForCounts(),
                                             _HICR_CHANNEL_HEAD_ADVANCE_COUNT_IDX * sizeof(size_t),
                                             sizeof(size_t));
    coordinationCommunicationManager->fence(getCoordinationBufferForCounts(), 1, 0);
  }

  /**
   * Identical to Producer::updateDepth(), but this coordination buffer
   * is larger and contains payload information as well as token metadata
   */
  __INLINE__ void updateDepth() {}

  /**
   * get payload buffer head position
   * @return payload buffer head position (in bytes)
   */
  [[nodiscard]] __INLINE__ size_t getPayloadHeadPosition() const noexcept { return getCircularBufferForPayloads()->getHeadPosition(); }

  /**
   * get the datatype size used for payload buffer
   * @return datatype size (in bytes) for payload buffer
   */
  __INLINE__ size_t getPayloadSize() { return _payloadSize; }

  /**
   * Get payload buffer depth
   * 
   * \return payload buffer depth (in bytes). It is the occupancy of the buffers
   * \note Even though there might be space for additional tokens in the payload buffer, it is not guaranteed that 
   *       the push() will succeed due to insufficient space in the coordination buffer
   */
  __INLINE__ size_t getPayloadDepth() { return getCircularBufferForPayloads()->getDepth(); }

  /**
   * get payload buffer capacity
   * @return payload buffer capacity (in bytes)
   */
  __INLINE__ size_t getPayloadCapacity() { return getCircularBufferForPayloads()->getCapacity(); }
  
  /**
   * Get depth of the coordination buffer of variable-size producer.
   * Because the current implementation first receives the payload (phase 1) before
   * receiving the message counts (phase 2), returning this depth should guarantee
   * we already have received the payloads
   * 
   * @return The number of elements in the variable-size producer channel
   * 
   * \note Even though there might be space for additional tokens in the coordination buffer, it is not guaranteed that 
   *       the push() will succeed due to insufficient space in the payload buffer
   */
  size_t getCoordinationDepth() { return getCircularBufferForCounts()->getDepth(); }

  /**
   * This function can be used to quickly check whether the channel is empty.
   *
   * It does not affects the internal state of the channel
   *
   * \returns true, if both message count and payload buffers are empty
   * \returns false, if one of the buffers is not empty
   * 
   */
  bool isEmpty() { return getCoordinationDepth() == 0; }

  /**
   * This funciton can be used to quickly check whether the channel is becoming full when trying 
   * to push an element of a given size. First thing, we are checking if we can still 
   * push tokens (i.e., if the coordination buffer has space). Second thing, we are checking the
   * payload buffer. If the current depth of the payload and the \ref requiredBufferSize to push 
   * exceed the channel capacity, the channel is considered full.
   * 
   * \param[in] requiredBufferSize size of the token to push into the channel
   * 
   * \return true if there is enough space to push the token, false otherwise
  */
  bool isFull(size_t requiredBufferSize)
  {
    // Check if we can push one more token
    auto coordinationCircularBuffer = getCircularBufferForCounts();
    if (coordinationCircularBuffer->getDepth() == coordinationCircularBuffer->getCapacity()) { return true; }

    // Check if there is enough space in the payload buffer. If
    auto payloadCircularBuffer = getCircularBufferForPayloads();
    if (payloadCircularBuffer->getDepth() + requiredBufferSize > payloadCircularBuffer->getCapacity()) { return true; }

    return false;
  }

  private:

  /**
   * Memory slot for payload buffer (allocated at consumer)
   */
  std::shared_ptr<GlobalMemorySlot> _payloadBuffer;
  /**
   * Memory slot for message size information (allocated at producer)
   */
  std::shared_ptr<LocalMemorySlot> _sizeInfoBuffer;
  /**
   * size of datatype for payload messages
   */
  size_t _payloadSize;

  /**
   * Memory slot that represents the token buffer that producer sends data to
   */
  const std::shared_ptr<GlobalMemorySlot> _tokenBuffer;

  /**
   * Global Memory slot pointing to the consumer coordination buffer for message size info
   */
  const std::shared_ptr<GlobalMemorySlot> _consumerCoordinationBufferForCounts;

  /**
   * Global Memory slot pointing to the consumer coordination buffer for payload info
   */
  const std::shared_ptr<GlobalMemorySlot> _consumerCoordinationBufferForPayloads;
};

} // namespace HiCR::channel::variableSize::SPSC
