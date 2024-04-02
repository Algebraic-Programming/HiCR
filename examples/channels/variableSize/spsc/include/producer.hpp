#pragma once

#include "common.hpp"
#include <hicr/core/L1/memoryManager.hpp>
#include <hicr/core/L1/communicationManager.hpp>
#include <hicr/frontends/channel/variableSize/spsc/producer.hpp>

void producerFc(HiCR::L1::MemoryManager               &memoryManager,
                HiCR::L1::CommunicationManager        &communicationManager,
                std::shared_ptr<HiCR::L0::MemorySpace> bufferMemorySpace,
                const size_t                           channelCapacity)
{
  // Getting required buffer size
  auto coordinationBufferSize = HiCR::channel::variableSize::Base::getCoordinationBufferSize();

  // Allocating sizes buffer as a local memory slot
  auto coordinationBufferForCounts = memoryManager.allocateLocalMemorySlot(bufferMemorySpace, coordinationBufferSize);

  auto coordinationBufferForPayloads = memoryManager.allocateLocalMemorySlot(bufferMemorySpace, coordinationBufferSize);

  auto sizeInfoBuffer = memoryManager.allocateLocalMemorySlot(bufferMemorySpace, sizeof(size_t));

  // Initializing coordination buffers for message sizes and payloads (sets to zero the counters)
  HiCR::channel::variableSize::Base::initializeCoordinationBuffer(coordinationBufferForCounts);
  HiCR::channel::variableSize::Base::initializeCoordinationBuffer(coordinationBufferForPayloads);

  // Exchanging local memory slots to become global for them to be used by the remote end
  communicationManager.exchangeGlobalMemorySlots(
    CHANNEL_TAG, {{PRODUCER_COORDINATION_BUFFER_FOR_SIZES_KEY, coordinationBufferForCounts}, {PRODUCER_COORDINATION_BUFFER_FOR_PAYLOADS_KEY, coordinationBufferForPayloads}});

  // Synchronizing so that all actors have finished registering their global memory slots
  communicationManager.fence(CHANNEL_TAG);

  // Obtaining the globally exchanged memory slots
  auto sizesBuffer                           = communicationManager.getGlobalMemorySlot(CHANNEL_TAG, SIZES_BUFFER_KEY);
  auto producerCoordinationBufferForCounts   = communicationManager.getGlobalMemorySlot(CHANNEL_TAG, PRODUCER_COORDINATION_BUFFER_FOR_SIZES_KEY);
  auto producerCoordinationBufferForPayloads = communicationManager.getGlobalMemorySlot(CHANNEL_TAG, PRODUCER_COORDINATION_BUFFER_FOR_PAYLOADS_KEY);
  auto payloadBuffer                         = communicationManager.getGlobalMemorySlot(CHANNEL_TAG, CONSUMER_PAYLOAD_KEY);

  // Creating producer and consumer channels
  auto producer = HiCR::channel::variableSize::SPSC::Producer(communicationManager,
                                                              sizeInfoBuffer,
                                                              payloadBuffer,
                                                              sizesBuffer,
                                                              coordinationBufferForCounts,
                                                              coordinationBufferForPayloads,
                                                              producerCoordinationBufferForCounts,
                                                              producerCoordinationBufferForPayloads,
                                                              PAYLOAD_CAPACITY,
                                                              sizeof(ELEMENT_TYPE),
                                                              channelCapacity);

  // Allocating a send slot to put the values we want to communicate
  ELEMENT_TYPE sendBuffer[4]  = {0, 1, 2, 3};
  ELEMENT_TYPE sendBuffer2[3] = {4, 5, 6};
  ELEMENT_TYPE sendBuffer3[2] = {7, 8};
  auto         sendBufferPtr  = &sendBuffer;
  auto         sendBuffer2Ptr = &sendBuffer2;
  auto         sendBuffer3Ptr = &sendBuffer3;
  auto         sendSlot       = memoryManager.registerLocalMemorySlot(bufferMemorySpace, sendBufferPtr, sizeof(sendBuffer));
  auto         sendSlot2      = memoryManager.registerLocalMemorySlot(bufferMemorySpace, sendBuffer2Ptr, sizeof(sendBuffer2));
  auto         sendSlot3      = memoryManager.registerLocalMemorySlot(bufferMemorySpace, sendBuffer3Ptr, sizeof(sendBuffer3));

  // Pushing first batch
  producer.push(sendSlot);
  Printer<ELEMENT_TYPE>::printBytes("PRODUCER sent:", sendBufferPtr, sizeof(sendBuffer), 0, sizeof(sendBuffer));

  // If channel is full, wait until it frees up
  while (!producer.isEmpty()) producer.updateDepth();

  // Sending second batch
  producer.push(sendSlot2);
  Printer<ELEMENT_TYPE>::printBytes("PRODUCER sent:", sendBuffer2Ptr, sizeof(sendBuffer2), 0, sizeof(sendBuffer2));

  // If channel is full, wait until it frees up
  while (!producer.isEmpty()) producer.updateDepth();

  // Sending third batch (same as first batch)
  producer.push(sendSlot3);
  Printer<ELEMENT_TYPE>::printBytes("PRODUCER sent:", sendBuffer3Ptr, sizeof(sendBuffer3), 0, sizeof(sendBuffer3));

  // Synchronizing so that all actors have finished registering their global memory slots
  communicationManager.fence(CHANNEL_TAG);

  // De-registering global slots (collective calls)
  communicationManager.deregisterGlobalMemorySlot(sizesBuffer);
  communicationManager.deregisterGlobalMemorySlot(producerCoordinationBufferForCounts);
  communicationManager.deregisterGlobalMemorySlot(producerCoordinationBufferForPayloads);

  // Freeing up local memory
  memoryManager.freeLocalMemorySlot(coordinationBufferForCounts);
  memoryManager.freeLocalMemorySlot(coordinationBufferForPayloads);
  memoryManager.freeLocalMemorySlot(sizeInfoBuffer);
}
