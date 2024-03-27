
/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file variableSize/spsc/producer.hpp
 * @brief Provides functionality for a var-size SPSC producer channel
 * @author K. Dichev & A. N. Yzelman & S. M Martin
 * @date 15/01/2024
 */

#pragma once

#include <hicr/frontends/channel/variableSize/base.hpp>

namespace HiCR
{

namespace channel
{

namespace variableSize
{

namespace SPSC
{

/**
 * Class definition for a HiCR Producer Channel
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
   * Memory slot that represents the token buffer that producer sends data to
   */
  const std::shared_ptr<L0::GlobalMemorySlot> _tokenBuffer;

  /**
   * Global Memory slot pointing to the producer's own coordination buffer for message size info
   */
  const std::shared_ptr<L0::GlobalMemorySlot> _producerCoordinationBufferForCounts;

  /**
   * Global Memory slot pointing to the producer's own coordination buffer for payload info
   */
  const std::shared_ptr<L0::GlobalMemorySlot> _producerCoordinationBufferForPayloads;

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
   *            The producer will push message sizes into this buffer, while there is enough space. This buffer should be big enough to
   *            hold at least one message size.
   * \param[in] internalCoordinationBufferForCounts This is a small buffer to hold the internal (local) state of the channel's message counts
   * \param[in] internalCoordinationBufferForPayloads This is a small buffer to hold the internal (local) state of the channel's payload sizes (in bytes)
   * \param[in] producerCoordinationBufferForCounts A global reference of the producer's own coordination buffer to check for updates on message counts
   * \param[in] producerCoordinationBufferForPayloads A global reference of the producer's own coordination buffer to check for updates on payload sizes (in bytes)
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
           std::shared_ptr<L0::GlobalMemorySlot> producerCoordinationBufferForCounts,
           std::shared_ptr<L0::GlobalMemorySlot> producerCoordinationBufferForPayloads,
           const size_t                          payloadCapacity,
           const size_t                          payloadSize,
           const size_t                          capacity)
    : variableSize::Base(communicationManager, internalCoordinationBufferForCounts, internalCoordinationBufferForPayloads, capacity, payloadCapacity),
      _payloadBuffer(payloadBuffer),
      _sizeInfoBuffer(sizeInfoBuffer),
      _payloadSize(payloadSize),
      _tokenBuffer(tokenBuffer),
      _producerCoordinationBufferForCounts(producerCoordinationBufferForCounts),
      _producerCoordinationBufferForPayloads(producerCoordinationBufferForPayloads)
  {}

  ~Producer() {}

  /**
   * Identical to Producer::updateDepth(), but this coordination buffer
   * is larger and contains payload information as well as token metadata
   */
  __INLINE__ void updateDepth()
  {
    _communicationManager->queryMemorySlotUpdates(_producerCoordinationBufferForCounts);
    _communicationManager->queryMemorySlotUpdates(_producerCoordinationBufferForPayloads);
  }

  /**
   * advance payload buffer tail by a number of bytes
   * @param[in] n bytes to advance payload buffer tail by
   */
  __INLINE__ void advancePayloadTail(const size_t n = 1) { _circularBufferForPayloads->advanceTail(n); }

  /**
   * advance payload buffer head by a number of bytes
   * @param[in] n bytes to advance payload buffer head by
   * @param[in] cachedDepth A boolean which is true if a cached depth should be used,
   * otherwise the current depth should be used. For details see \ref CircularBuffer::advanceHead
   * documentation
   */
  void advancePayloadHead(const size_t n = 1, bool cachedDepth = false) { _circularBufferForPayloads->advanceHead(n, cachedDepth); }
  /**
   * get payload buffer head position
   * @return payload buffer head position (in bytes)
   */
  __INLINE__ size_t getPayloadHeadPosition() const noexcept { return _circularBufferForPayloads->getHeadPosition(); }

  /**
   * get the datatype size used for payload buffer
   * @return datatype size (in bytes) for payload buffer
   */
  __INLINE__ size_t getPayloadSize() { return _payloadSize; }

  /**
   * get payload buffer depth
   * @return payload buffer depth (in bytes)
   */
  __INLINE__ size_t getPayloadDepth() { return _circularBufferForPayloads->getDepth(); }

  /**
   * get payload buffer capacity
   * @return payload buffer capacity (in bytes)
   */
  __INLINE__ size_t getPayloadCapacity() { return _circularBufferForPayloads->getCapacity(); }

  /**
   * Puts new variable-sized messages unto the channel.
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
  __INLINE__ void push(std::shared_ptr<L0::LocalMemorySlot> sourceSlot, const size_t n = 1)
  {
    if (n != 1) HICR_THROW_RUNTIME("HiCR currently has no implementation for n != 1 with push(sourceSlot, n) for variable size version.");

    // Make sure source slot is beg enough to satisfy the operation
    size_t requiredBufferSize     = sourceSlot->getSize();
    size_t providedBufferCapacity = getPayloadCapacity();

    // Updating depth of token (message sizes) and payload buffers
    updateDepth();
    if (getPayloadDepth() + requiredBufferSize > providedBufferCapacity)
      HICR_THROW_RUNTIME("Attempting to push (%lu) bytes while the channel currently has depth (%lu). This would exceed capacity (%lu).\n",
                         requiredBufferSize,
                         getPayloadDepth(),
                         providedBufferCapacity);

    size_t *sizeInfoBufferPtr = static_cast<size_t *>(_sizeInfoBuffer->getPointer());
    sizeInfoBufferPtr[0]      = requiredBufferSize;

    // If the exchange buffer does not have n free slots, reject the operation
    if (getDepth() + 1 > _circularBuffer->getCapacity())
      HICR_THROW_RUNTIME(
        "Attempting to push with (%lu) tokens while the channel has (%lu) tokens and this would exceed capacity (%lu).\n", 1, getDepth(), _circularBuffer->getCapacity());

    _circularBuffer->setCachedDepth(_circularBuffer->getDepth());
    // Advance head, as we have added new elements
    // It is important to do the advanceHead and copy together,
    // or else issues such as advancing head index too early or too late might occur!
    _communicationManager->memcpy(_tokenBuffer,                                        /* destination */
                                  getTokenSize() * _circularBuffer->getHeadPosition(), /* dst_offset */
                                  _sizeInfoBuffer,                                     /* source */
                                  0,                                                   /* src_offset */
                                  getTokenSize());                                     /* size */
    _communicationManager->fence(_sizeInfoBuffer, 1, 0);
    _circularBuffer->advanceHead(1, true);

    /**
     * Payload copy:
     *  - We have checked (requiredBufferSize  <= depth)
     *  that the payload fits into available circular buffer,
     *  but it is possible it spills over the end into the
     *  beginning. Cover this corner case below
     *
     */
    _circularBufferForPayloads->setCachedDepth(getPayloadDepth());
    if (requiredBufferSize + getPayloadHeadPosition() > getPayloadCapacity())
    {
      size_t first_chunk  = getPayloadCapacity() - getPayloadHeadPosition();
      size_t second_chunk = requiredBufferSize - first_chunk;

      // copy first part to end of buffer
      _communicationManager->memcpy(_payloadBuffer,           /* destination */
                                    getPayloadHeadPosition(), /* dst_offset */
                                    sourceSlot,               /* source */
                                    0,                        /* src_offset */
                                    first_chunk);             /* size */
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
      _circularBufferForPayloads->setCachedDepth(getPayloadDepth());
      _communicationManager->memcpy(_payloadBuffer, getPayloadHeadPosition(), sourceSlot, 0, requiredBufferSize);
      _communicationManager->fence(sourceSlot, 1, 0);
    }
    advancePayloadHead(requiredBufferSize, true);
  }

  /**
   * get depth of variable-size producer
   * @return depth of variable-size producer
   */
  size_t getDepth()
  {
    // Because the current implementation first receives the message size in the token buffer, followed
    // by the message payload, it is possible for the token buffer to have a larged depth (by 1) than the payload buffer. Therefore, we need to return the minimum of the two depths
    return std::min(_circularBuffer->getDepth(), _circularBufferForPayloads->getDepth() / getPayloadSize());
  }

  /**
   * This function can be used to quickly check whether the channel is empty.
   *
   * It does not affects the internal state of the channel
   *
   * \returns true, if both message count and payload buffers are empty
   * \returns false, if one of the buffers is not empty
   */
  bool isEmpty() { return (_circularBuffer->getDepth() == 0) && (_circularBufferForPayloads->getDepth() == 0); }
};

} // namespace SPSC

} // namespace variableSize

} // namespace channel

} // namespace HiCR
