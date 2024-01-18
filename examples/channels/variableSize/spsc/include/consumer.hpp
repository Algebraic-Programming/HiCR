#pragma once

#include "common.hpp"
#include <hicr/L1/memoryManager.hpp>
#include <hicr/L1/communicationManager.hpp>
#include <frontends/channel/variableSize/spsc/consumer.hpp>

void consumerFc(HiCR::L1::MemoryManager &memoryManager, HiCR::L1::CommunicationManager &communicationManager, std::shared_ptr<HiCR::L0::MemorySpace> bufferMemorySpace, const size_t channelCapacity)
{
  // Getting required buffer sizes
  auto sizesBufferSize = HiCR::channel::variableSize::Base::getTokenBufferSize(sizeof(size_t), channelCapacity);

  // Allocating sizes buffer as a local memory slot
  auto sizesBufferSlot = memoryManager.allocateLocalMemorySlot(bufferMemorySpace, sizesBufferSize);

  // Allocating payload buffer as a local memory slot
  auto payloadBufferSlot = memoryManager.allocateLocalMemorySlot(bufferMemorySpace, PAYLOAD_CAPACITY);

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
  communicationManager.exchangeGlobalMemorySlots(CHANNEL_TAG, {{SIZES_BUFFER_KEY, sizesBufferSlot}, {CONSUMER_COORDINATION_BUFFER_FOR_SIZES_KEY, coordinationBufferForCounts}, {CONSUMER_COORDINATION_BUFFER_FOR_PAYLOADS_KEY, coordinationBufferForPayloads}, {CONSUMER_PAYLOAD_KEY, payloadBufferSlot}});

  // Synchronizing so that all actors have finished registering their global memory slots
  communicationManager.fence(CHANNEL_TAG);

  // Obtaining the globally exchanged memory slots
  std::shared_ptr<HiCR::L0::GlobalMemorySlot> globalSizesBufferSlot = communicationManager.getGlobalMemorySlot(CHANNEL_TAG, SIZES_BUFFER_KEY);
  auto producerCoordinationBufferForCounts = communicationManager.getGlobalMemorySlot(CHANNEL_TAG, PRODUCER_COORDINATION_BUFFER_FOR_SIZES_KEY);
  auto producerCoordinationBufferForPayloads = communicationManager.getGlobalMemorySlot(CHANNEL_TAG, PRODUCER_COORDINATION_BUFFER_FOR_PAYLOADS_KEY);
  auto payloadBuffer = communicationManager.getGlobalMemorySlot(CHANNEL_TAG, CONSUMER_PAYLOAD_KEY);

  // Creating producer and consumer channels
  auto consumer = HiCR::channel::SPSC::Consumer(communicationManager, payloadBuffer /*payload buffer */, globalSizesBufferSlot, coordinationBufferForCounts, coordinationBufferForPayloads, producerCoordinationBufferForCounts, producerCoordinationBufferForPayloads, PAYLOAD_CAPACITY, sizeof(ELEMENT_TYPE), channelCapacity);

  // Getting a single value from the channel
  while (consumer.getDepth() != 1) consumer.updateDepth();

  // Getting internal pointer of the token buffer slot
  auto payloadBufferPtr = (ELEMENT_TYPE *)payloadBufferSlot->getPointer();

  // Static knowledge about # elements to be popped
  size_t poppedElems = 0;
  while (poppedElems < 3)
  {
    while (consumer.isEmpty()) consumer.updateDepth();
    auto res = consumer.peek();
    Printer<ELEMENT_TYPE>::printBytes("CONSUMER:", payloadBufferPtr, PAYLOAD_CAPACITY, res[0], res[1]);
    consumer.pop();
    poppedElems++;
  }

  // Synchronizing so that all actors have finished registering their global memory slots
  communicationManager.fence(CHANNEL_TAG);

  // De-registering global slots (collective calls)
  communicationManager.deregisterGlobalMemorySlot(globalSizesBufferSlot);
  communicationManager.deregisterGlobalMemorySlot(producerCoordinationBufferForCounts);
  communicationManager.deregisterGlobalMemorySlot(producerCoordinationBufferForPayloads);
  // Freeing up local memory
  memoryManager.freeLocalMemorySlot(payloadBufferSlot);
  memoryManager.freeLocalMemorySlot(sizesBufferSlot);
  memoryManager.freeLocalMemorySlot(coordinationBufferForCounts);
  memoryManager.freeLocalMemorySlot(coordinationBufferForPayloads);
}
