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
 * @file variableSize/mpsc/locking/producer.hpp
 * @brief Provides variable-sized MPSC producer channel, locking version
 * @author O. Korakitis & K. Dichev
 * @date 04/06/2024
 */

#pragma once

#include <hicr/frontends/channel/variableSize/base.hpp>
#include <utility>

namespace HiCR::channel::variableSize::MPSC::locking
{

/**
 * Class definition for a HiCR MPSC Producer Channel
 *
 * It exposes the functionality to be expected for a producer channel
 *
 */
class Producer final : public variableSize::Base
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
   *            buffer, while there is enough space. This buffer should be big enough to hold at least the largest of the variable-size messages.
   * \param[in] tokenBuffer The memory slot pertaining to the token buffer, which is used to hold message size data.
   *            The producer will push message sizes into this buffer, while there is enough space. This buffer should be big enough
   *            to hold at least one message size.
   * \param[in] internalCoordinationBufferForCounts This is a small buffer to hold the internal (local) state of the channel's message counts
   * \param[in] internalCoordinationBufferForPayloads This is a small buffer to hold the internal (local) state of the channel's payload sizes (in bytes)
   * \param[in] consumerCoordinationBufferForCounts A global reference of the consumer's coordination buffer to check for updates on message counts
   * \param[in] consumerCoordinationBufferForPayloads A global reference of the consumer's coordination buffer to check for updates on payload sizes (in bytes)
   * \param[in] payloadCapacity capacity in bytes of the buffer for message payloads
   * \param[in] payloadSize size in bytes of the datatype used for variable-sized messages
   * \param[in] capacity The maximum number of tokens that will be held by this channel
   */
  Producer(CommunicationManager                   &communicationManager,
           std::shared_ptr<LocalMemorySlot>        sizeInfoBuffer,
           std::shared_ptr<GlobalMemorySlot>       payloadBuffer,
           std::shared_ptr<GlobalMemorySlot>       tokenBuffer,
           const std::shared_ptr<LocalMemorySlot> &internalCoordinationBufferForCounts,
           const std::shared_ptr<LocalMemorySlot> &internalCoordinationBufferForPayloads,
           std::shared_ptr<GlobalMemorySlot>       consumerCoordinationBufferForCounts,
           std::shared_ptr<GlobalMemorySlot>       consumerCoordinationBufferForPayloads,
           const size_t                                payloadCapacity,
           const size_t                                payloadSize,
           const size_t                                capacity)
    : variableSize::Base(communicationManager, internalCoordinationBufferForCounts, internalCoordinationBufferForPayloads, capacity, payloadCapacity),
      _payloadBuffer(std::move(payloadBuffer)),
      _sizeInfoBuffer(std::move(sizeInfoBuffer)),
      _payloadSize(payloadSize),
      _tokenSizeBuffer(std::move(tokenBuffer)),
      _consumerCoordinationBufferForCounts(std::move(consumerCoordinationBufferForCounts)),
      _consumerCoordinationBufferForPayloads(std::move(consumerCoordinationBufferForPayloads))
  {}

  ~Producer() = default;

  /**
   * This call gets the head/tail indices from the consumer.
   * The assumption is we hold the global lock.
   */
  __INLINE__ void updateDepth() // NOTE: we DO know we have the lock!!!!
  {
    getCommunicationManager()->memcpy(getCoordinationBufferForCounts(),                            /* destination */
                                      0,                                                           /* dst_offset */
                                      _consumerCoordinationBufferForCounts,                        /* source */
                                      0,                                                           /* src_offset */
                                      2 * sizeof(_HICR_CHANNEL_COORDINATION_BUFFER_ELEMENT_TYPE)); /* size */

    getCommunicationManager()->memcpy(getCoordinationBufferForPayloads(),                          /* destination */
                                      0,                                                           /* dst_offset */
                                      _consumerCoordinationBufferForPayloads,                      /* source */
                                      0,                                                           /* src_offset */
                                      2 * sizeof(_HICR_CHANNEL_COORDINATION_BUFFER_ELEMENT_TYPE)); /* size */

    getCommunicationManager()->fence(getCoordinationBufferForCounts(), 0, 1);
    getCommunicationManager()->fence(getCoordinationBufferForPayloads(), 0, 1);
    /**
     * Now we know the exact buffer state at the consumer
     */
  }

  /**
   * get the datatype size used for payload buffer
   * @return datatype size (in bytes) for payload buffer
   */
  inline size_t getPayloadSize() { return _payloadSize; }

  /**
   * get payload buffer depth
   * @return payload buffer depth (in bytes)
   */
  inline size_t getPayloadDepth() { return getCircularBufferForPayloads()->getDepth(); }

  /**
   * Puts new variable-sized messages unto the channel.
   *
   * This is a one-sided blocking primitive that need not be made collectively.
   *
   * @param[in] sourceSlot  Source slot (buffer) to copy into buffer
   * @param[in] n  Number of times to copy this \p sourceSlot into buffer
   * @return True, if successful. False, if not (e.g., could not get ahold of the memory slot lock)
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
  __INLINE__ bool push(const std::shared_ptr<LocalMemorySlot> &sourceSlot, const size_t n = 1)
  {
    if (n != 1) HICR_THROW_RUNTIME("HiCR currently has no implementation for n != 1 with push(sourceSlot, n) for variable size version.");

    // Make sure source slot is big enough to satisfy the operation
    size_t requiredBufferSize = sourceSlot->getSize();

    // Flag to record whether the operation was successful or not (it simplifies code by releasing locks only once)
    bool successFlag = false;

    // Locking remote token and coordination buffer slots
    if (getCommunicationManager()->acquireGlobalLock(_consumerCoordinationBufferForCounts) == false) return successFlag;

    // Updating depth of token (message sizes) and payload buffers from the consumer
    updateDepth();

    // Check if the amount of data we wish to write (requiredBufferSize)
    // would fit in the consumer payload buffer in its current state.
    // If not, reject the operation
    if (getCircularBufferForPayloads()->getDepth() + requiredBufferSize > getCircularBufferForPayloads()->getCapacity())
    {
      getCommunicationManager()->releaseGlobalLock(_consumerCoordinationBufferForCounts);
      return successFlag;
    }

    auto *sizeInfoBufferPtr = static_cast<size_t *>(_sizeInfoBuffer->getPointer());
    sizeInfoBufferPtr[0]    = requiredBufferSize;

    // Check if the consumer buffer has n free slots. If not, reject the operation
    if (getCircularBufferForCounts()->getDepth() + 1 > getCircularBufferForCounts()->getCapacity())
    {
      getCommunicationManager()->releaseGlobalLock(_consumerCoordinationBufferForCounts);
      return successFlag;
    }

    /**
     * Phase 1: Update the size (in bytes) of the pending payload
     * at the consumer
     */
    getCommunicationManager()->memcpy(_tokenSizeBuffer,                                                 /* destination */
                                      getTokenSize() * getCircularBufferForCounts()->getHeadPosition(), /* dst_offset */
                                      _sizeInfoBuffer,                                                  /* source */
                                      0,                                                                /* src_offset */
                                      getTokenSize());                                                  /* size */
    getCommunicationManager()->fence(_sizeInfoBuffer, 1, 0);
    successFlag = true;

    /**
     * Phase 2: Payload copy:
     *  - We have checked (requiredBufferSize  <= depth)
     *  that the payload fits into available circular buffer,
     *  but it is possible it spills over the end into the
     *  beginning. Cover this corner case below
     *
     */
    if (requiredBufferSize + getCircularBufferForPayloads()->getHeadPosition() > getCircularBufferForPayloads()->getCapacity())
    {
      size_t first_chunk  = getCircularBufferForPayloads()->getCapacity() - getCircularBufferForPayloads()->getHeadPosition();
      size_t second_chunk = requiredBufferSize - first_chunk;
      // copy first part to end of buffer
      getCommunicationManager()->memcpy(_payloadBuffer,                                    /* destination */
                                        getCircularBufferForPayloads()->getHeadPosition(), /* dst_offset */
                                        sourceSlot,                                        /* source */
                                        0,                                                 /* src_offset */
                                        first_chunk);                                      /* size */
      // copy second part to beginning of buffer
      getCommunicationManager()->memcpy(_payloadBuffer, /* destination */
                                        0,              /* dst_offset */
                                        sourceSlot,     /* source */
                                        first_chunk,    /* src_offset */
                                        second_chunk);  /* size */

      getCommunicationManager()->fence(sourceSlot, 2, 0);
    }
    else
    {
      getCommunicationManager()->memcpy(_payloadBuffer, getCircularBufferForPayloads()->getHeadPosition(), sourceSlot, 0, requiredBufferSize);
      getCommunicationManager()->fence(sourceSlot, 1, 0);
    }

    // Remotely push an element into consumer side, updating consumer head indices
    getCircularBufferForCounts()->advanceHead(1);
    getCircularBufferForPayloads()->advanceHead(requiredBufferSize);

    // only update head index at consumer (byte size = one buffer element)
    getCommunicationManager()->memcpy(_consumerCoordinationBufferForCounts, 0, getCoordinationBufferForCounts(), 0, sizeof(_HICR_CHANNEL_COORDINATION_BUFFER_ELEMENT_TYPE));
    // only update head index at consumer (byte size = one buffer element)
    getCommunicationManager()->memcpy(_consumerCoordinationBufferForPayloads, 0, getCoordinationBufferForPayloads(), 0, sizeof(_HICR_CHANNEL_COORDINATION_BUFFER_ELEMENT_TYPE));
    // backend LPF needs this to complete
    getCommunicationManager()->fence(getCoordinationBufferForCounts(), 1, 0);
    getCommunicationManager()->fence(getCoordinationBufferForPayloads(), 1, 0);

    getCommunicationManager()->releaseGlobalLock(_consumerCoordinationBufferForCounts);

    return successFlag;
  }

  /**
   * get depth of variable-size producer
   * @return depth of variable-size producer
   */
  __INLINE__ size_t getDepth()
  {
    // Because the current implementation first receives the message size in the token buffer, followed
    // by the message payload, it is possible for the token buffer to have a larged depth (by 1) than the payload buffer.
    // Therefore, we need to return the minimum of the two depths
    return std::min(getCircularBufferForCounts()->getDepth(), getCircularBufferForPayloads()->getDepth() / getPayloadSize());
  }

  /**
   * This function can be used to quickly check whether the channel is empty.
   *
   * It does not affect the internal state of the channel
   *
   * \returns true, if both message count and payload buffers are empty
   * \returns false, if one of the buffers is not empty
   */
  __INLINE__ bool isEmpty() { return (getCircularBufferForCounts()->getDepth() == 0) && (getCircularBufferForPayloads()->getDepth() == 0); }

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
   * Memory slot that represents the token buffer that producer sends data to (about variable size)
   */
  const std::shared_ptr<GlobalMemorySlot> _tokenSizeBuffer;

  /**
   * Global Memory slot pointing to the consumer's coordination buffer for message size info
   */
  const std::shared_ptr<GlobalMemorySlot> _consumerCoordinationBufferForCounts;

  /**
   * Global Memory slot pointing to the consumer's coordination buffer for payload info
   */
  const std::shared_ptr<GlobalMemorySlot> _consumerCoordinationBufferForPayloads;
};

} // namespace HiCR::channel::variableSize::MPSC::locking
