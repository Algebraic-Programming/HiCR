#pragma once

#include <hicr/core/L1/memoryManager.hpp>
#include <hicr/core/L1/communicationManager.hpp>
#include <hicr/frontends/channel/variableSize/mpsc/locking/consumer.hpp>
#include "common.hpp"

void consumerFc(HiCR::L1::MemoryManager               &memoryManager,
                HiCR::L1::CommunicationManager        &communicationManager,
                std::shared_ptr<HiCR::L0::MemorySpace> bufferMemorySpace,
                const size_t                           channelCapacity,
                const size_t                           producerCount)
{
  // Getting required buffer sizes
  auto sizesBufferSize = HiCR::channel::variableSize::Base::getTokenBufferSize(sizeof(size_t), channelCapacity);

  // Allocating sizes buffer as a local memory slot
  auto sizesBufferSlot = memoryManager.allocateLocalMemorySlot(bufferMemorySpace, sizesBufferSize);

  const size_t payloadCapacity = channelCapacity * sizeof(ELEMENT_TYPE);

  // Allocating payload buffer as a local memory slot
  auto payloadBufferSlot = memoryManager.allocateLocalMemorySlot(bufferMemorySpace, payloadCapacity);

  // Getting required buffer size
  auto coordinationBufferSize = HiCR::channel::variableSize::Base::getCoordinationBufferSize();

  // Allocating coordination buffer for internal message size metadata
  auto coordinationBufferForCounts = memoryManager.allocateLocalMemorySlot(bufferMemorySpace, coordinationBufferSize);

  // Allocating coordination buffer for internal payload metadata
  auto coordinationBufferForPayloads = memoryManager.allocateLocalMemorySlot(bufferMemorySpace, coordinationBufferSize);

  // Initializing coordination buffer (sets to zero the counters)
  HiCR::channel::variableSize::Base::initializeCoordinationBuffer(coordinationBufferForCounts);

  HiCR::channel::variableSize::Base::initializeCoordinationBuffer(coordinationBufferForPayloads);

  // Exchanging local memory slots to become global for them to be used by the remote end
  communicationManager.exchangeGlobalMemorySlots(CHANNEL_TAG,
                                                 {{SIZES_BUFFER_KEY, sizesBufferSlot},
                                                  {CONSUMER_COORDINATION_BUFFER_FOR_SIZES_KEY, coordinationBufferForCounts},
                                                  {CONSUMER_COORDINATION_BUFFER_FOR_PAYLOADS_KEY, coordinationBufferForPayloads},
                                                  {CONSUMER_PAYLOAD_KEY, payloadBufferSlot}});

  // Synchronizing so that all actors have finished registering their global memory slots
  communicationManager.fence(CHANNEL_TAG);

  // Obtaining the globally exchanged memory slots
  auto globalSizesBufferSlot                 = communicationManager.getGlobalMemorySlot(CHANNEL_TAG, SIZES_BUFFER_KEY);
  auto consumerCoordinationBufferForCounts   = communicationManager.getGlobalMemorySlot(CHANNEL_TAG, CONSUMER_COORDINATION_BUFFER_FOR_SIZES_KEY);
  auto consumerCoordinationBufferForPayloads = communicationManager.getGlobalMemorySlot(CHANNEL_TAG, CONSUMER_COORDINATION_BUFFER_FOR_PAYLOADS_KEY);
  auto globalPayloadBuffer                   = communicationManager.getGlobalMemorySlot(CHANNEL_TAG, CONSUMER_PAYLOAD_KEY);

  // Creating producer and consumer channels
  auto consumer = HiCR::channel::variableSize::MPSC::locking::Consumer(communicationManager,
                                                                       globalPayloadBuffer,   /* payloadBuffer */
                                                                       globalSizesBufferSlot, /* tokenSizeBuffer */
                                                                       coordinationBufferForCounts,
                                                                       coordinationBufferForPayloads,
                                                                       consumerCoordinationBufferForCounts,
                                                                       consumerCoordinationBufferForPayloads,
                                                                       payloadCapacity,
                                                                       sizeof(ELEMENT_TYPE), /* payloadSize */
                                                                       channelCapacity);

  // Getting internal pointer of the token buffer slot
  auto payloadBufferPtr = (ELEMENT_TYPE *)payloadBufferSlot->getPointer();

  // Calculating the expected message count
  size_t expectedMessageCount = MESSAGES_PER_PRODUCER * producerCount;

  size_t poppedElems = 0;
  char   prefix[64]  = {'\0'};
  while (poppedElems < expectedMessageCount)
  {
    while (consumer.isEmpty())
      ; // spin
    auto res = consumer.peek();
    /**
     * An example-specific way to deduce the sender rank
     */
    sprintf(prefix, "CONSUMER receives from PRODUCER %u:", payloadBufferPtr[res[0] / sizeof(ELEMENT_TYPE)]);

    Printer<ELEMENT_TYPE>::printBytes(prefix, payloadBufferPtr, payloadCapacity, res[0], res[1]);

    while (consumer.pop() == false)
      ;

    poppedElems++;
  }

  // Synchronizing so that all actors have finished registering their global memory slots
  communicationManager.fence(CHANNEL_TAG);

  // De-registering global slots (collective calls)
  communicationManager.deregisterGlobalMemorySlot(globalSizesBufferSlot);
  communicationManager.deregisterGlobalMemorySlot(consumerCoordinationBufferForCounts);
  communicationManager.deregisterGlobalMemorySlot(consumerCoordinationBufferForPayloads);
  // Freeing up local memory
  memoryManager.freeLocalMemorySlot(payloadBufferSlot);
  memoryManager.freeLocalMemorySlot(sizesBufferSlot);
  memoryManager.freeLocalMemorySlot(coordinationBufferForCounts);
  memoryManager.freeLocalMemorySlot(coordinationBufferForPayloads);
}
