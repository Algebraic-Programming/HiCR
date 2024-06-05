
/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file variableSize/mpsc/locking/producer.hpp
 * @brief Provides functionality for a var-size MPSC producer channel
 * @author O. Korakitis & K. Dichev
 * @date 04/06/2024
 */

#pragma once

#include <hicr/frontends/channel/variableSize/base.hpp>

namespace HiCR
{

namespace channel
{

namespace variableSize
{

namespace MPSC
{

namespace locking
{

/**
 * Class definition for a HiCR MPSC Producer Channel
 *
 * It exposes the functionality to be expected for a producer channel
 *
 */
class Producer final : public variableSize::Base
{
  protected:

  /**
   * Memory slot for payload buffer (allocated at consumer)
   */
  std::shared_ptr<L0::GlobalMemorySlot> _payloadBuffer;

  /**
   * Memory slot for message size information (allocated at producer)
   */
  std::shared_ptr<L0::LocalMemorySlot> _sizeInfoBuffer;

  /**
   * size of datatype for payload messages
   */
  size_t _payloadSize;

  /**
   * Memory slot that represents the token buffer that producer sends data to (about variable size)
   */
  const std::shared_ptr<L0::GlobalMemorySlot> _tokenSizeBuffer;

  /**
   * Global Memory slot pointing to the consumer's coordination buffer for message size info
   */
  const std::shared_ptr<L0::GlobalMemorySlot> _consumerCoordinationBufferForCounts;

  /**
   * Global Memory slot pointing to the consumer's coordination buffer for payload info
   */
  const std::shared_ptr<L0::GlobalMemorySlot> _consumerCoordinationBufferForPayloads;

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
    : variableSize::Base(communicationManager, internalCoordinationBufferForCounts, internalCoordinationBufferForPayloads, capacity, payloadCapacity),
      _payloadBuffer(payloadBuffer),
      _sizeInfoBuffer(sizeInfoBuffer),
      _payloadSize(payloadSize),
      _tokenSizeBuffer(tokenBuffer),
      _consumerCoordinationBufferForCounts(consumerCoordinationBufferForCounts),
      _consumerCoordinationBufferForPayloads(consumerCoordinationBufferForPayloads)
  {}

  ~Producer() {}

  /**
   * This call gets the head/tail indices from the consumer.
   * The assumption is we hold the global lock.
   */
  __INLINE__ void updateDepth() // NOTE: we DO know we have the lock!!!!
  {
    _communicationManager->memcpy(_coordinationBufferForCounts,                                /* destination */
                                  0,                                                           /* dst_offset */
                                  _consumerCoordinationBufferForCounts,                        /* source */
                                  0,                                                           /* src_offset */
                                  2 * sizeof(_HICR_CHANNEL_COORDINATION_BUFFER_ELEMENT_TYPE)); /* size */

    _communicationManager->memcpy(_coordinationBufferForPayloads,                              /* destination */
                                  0,                                                           /* dst_offset */
                                  _consumerCoordinationBufferForPayloads,                      /* source */
                                  0,                                                           /* src_offset */
                                  2 * sizeof(_HICR_CHANNEL_COORDINATION_BUFFER_ELEMENT_TYPE)); /* size */

    _communicationManager->fence(_coordinationBufferForCounts, 0, 1);
    _communicationManager->fence(_coordinationBufferForPayloads, 0, 1);
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
  inline size_t getPayloadDepth() { return _circularBufferForPayloads->getDepth(); }

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
  __INLINE__ bool push(std::shared_ptr<L0::LocalMemorySlot> sourceSlot, const size_t n = 1)
  {
    if (n != 1) HICR_THROW_RUNTIME("HiCR currently has no implementation for n != 1 with push(sourceSlot, n) for variable size version.");

    // Make sure source slot is big enough to satisfy the operation
    size_t requiredBufferSize = sourceSlot->getSize();

    // Flag to record whether the operation was successful or not (it simplifies code by releasing locks only once)
    bool successFlag = false;

    // Locking remote token and coordination buffer slots
    if (_communicationManager->acquireGlobalLock(_consumerCoordinationBufferForCounts) == false) return successFlag;

    // Updating depth of token (message sizes) and payload buffers from the consumer
    updateDepth();

    // Check if the amount of data we wish to write (requiredBufferSize)
    // would fit in the consumer payload buffer in its current state.
    // If not, reject the operation
    if (_circularBufferForPayloads->getDepth() + requiredBufferSize > _circularBufferForPayloads->getCapacity())
    {
      _communicationManager->releaseGlobalLock(_consumerCoordinationBufferForCounts);
      return successFlag;
    }

    size_t *sizeInfoBufferPtr = static_cast<size_t *>(_sizeInfoBuffer->getPointer());
    sizeInfoBufferPtr[0]      = requiredBufferSize;

    // Check if the consumer buffer has n free slots. If not, reject the operation
    if (_circularBufferForCounts->getDepth() + 1 > _circularBufferForCounts->getCapacity())
    {
      _communicationManager->releaseGlobalLock(_consumerCoordinationBufferForCounts);
      return successFlag;
    }

    /**
     * Phase 1: Update the size (in bytes) of the pending payload
     * at the consumer
     */
    _communicationManager->memcpy(_tokenSizeBuffer,                                             /* destination */
                                  getTokenSize() * _circularBufferForCounts->getHeadPosition(), /* dst_offset */
                                  _sizeInfoBuffer,                                              /* source */
                                  0,                                                            /* src_offset */
                                  getTokenSize());                                              /* size */
    _communicationManager->fence(_sizeInfoBuffer, 1, 0);
    successFlag = true;

    /**
     * Phase 2: Payload copy:
     *  - We have checked (requiredBufferSize  <= depth)
     *  that the payload fits into available circular buffer,
     *  but it is possible it spills over the end into the
     *  beginning. Cover this corner case below
     *
     */
    if (requiredBufferSize + _circularBufferForPayloads->getHeadPosition() > _circularBufferForPayloads->getCapacity())
    {
      size_t first_chunk  = _circularBufferForPayloads->getCapacity() - _circularBufferForPayloads->getHeadPosition();
      size_t second_chunk = requiredBufferSize - first_chunk;
      // copy first part to end of buffer
      _communicationManager->memcpy(_payloadBuffer,                                /* destination */
                                    _circularBufferForPayloads->getHeadPosition(), /* dst_offset */
                                    sourceSlot,                                    /* source */
                                    0,                                             /* src_offset */
                                    first_chunk);                                  /* size */
      // copy second part to beginning of buffer
      _communicationManager->memcpy(_payloadBuffer, /* destination */
                                    0,              /* dst_offset */
                                    sourceSlot,     /* source */
                                    first_chunk,    /* src_offset */
                                    second_chunk);  /* size */

      _communicationManager->fence(sourceSlot, 2, 0);
    }
    else
    {
      _communicationManager->memcpy(_payloadBuffer, _circularBufferForPayloads->getHeadPosition(), sourceSlot, 0, requiredBufferSize);
      _communicationManager->fence(sourceSlot, 1, 0);
    }

    // Remotely push an element into consumer side, updating consumer head indices
    _circularBufferForCounts->advanceHead(1);
    _circularBufferForPayloads->advanceHead(requiredBufferSize);

    // only update head index at consumer (byte size = one buffer element)
    _communicationManager->memcpy(_consumerCoordinationBufferForCounts, 0, _coordinationBufferForCounts, 0, sizeof(_HICR_CHANNEL_COORDINATION_BUFFER_ELEMENT_TYPE));
    // only update head index at consumer (byte size = one buffer element)
    _communicationManager->memcpy(_consumerCoordinationBufferForPayloads, 0, _coordinationBufferForPayloads, 0, sizeof(_HICR_CHANNEL_COORDINATION_BUFFER_ELEMENT_TYPE));
    // backend LPF needs this to complete
    _communicationManager->fence(_coordinationBufferForCounts, 1, 0);
    _communicationManager->fence(_coordinationBufferForPayloads, 1, 0);

    _communicationManager->releaseGlobalLock(_consumerCoordinationBufferForCounts);

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
    return std::min(_circularBufferForCounts->getDepth(), _circularBufferForPayloads->getDepth() / getPayloadSize());
  }

  /**
   * This function can be used to quickly check whether the channel is empty.
   *
   * It does not affect the internal state of the channel
   *
   * \returns true, if both message count and payload buffers are empty
   * \returns false, if one of the buffers is not empty
   */
  __INLINE__ bool isEmpty() { return (_circularBufferForCounts->getDepth() == 0) && (_circularBufferForPayloads->getDepth() == 0); }
};

} // namespace locking

} // namespace MPSC

} // namespace variableSize

} // namespace channel

} // namespace HiCR
