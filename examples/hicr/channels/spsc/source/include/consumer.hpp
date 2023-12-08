#pragma once

#include <hicr/L1/memoryManager.hpp>
#include <hicr/L1/channel/spsc/consumer.hpp>
#include "common.hpp"

void consumerFc(HiCR::L1::MemoryManager *memoryManager, const size_t channelCapacity)
{
 // Getting required buffer sizes
 auto tokenBufferSize = HiCR::L1::channel::Base::getTokenBufferSize(sizeof(ELEMENT_TYPE), channelCapacity);

 // Obtaining memory spaces
 auto memSpaces = memoryManager->getMemorySpaceList();

 // Allocating token buffer as a local memory slot
 auto tokenBufferSlot = memoryManager->allocateLocalMemorySlot(*memSpaces.begin(), tokenBufferSize);

 // Getting required buffer size
 auto coordinationBufferSize = HiCR::L1::channel::Base::getCoordinationBufferSize();

 // Allocating coordination buffer as a local memory slot
 auto consumerCoordinationBuffer = memoryManager->allocateLocalMemorySlot(*memSpaces.begin(), coordinationBufferSize);

  // Initializing coordination buffer (sets to zero the counters)
 HiCR::L1::channel::Base::initializeCoordinationBuffer(consumerCoordinationBuffer);

 // Exchanging local memory slots to become global for them to be used by the remote end
 memoryManager->exchangeGlobalMemorySlots(CHANNEL_TAG, {{TOKEN_BUFFER_KEY, tokenBufferSlot}});

 // Synchronizing so that all actors have finished registering their global memory slots
 memoryManager->fence(CHANNEL_TAG);

 // Obtaining the globally exchanged memory slots
 auto globalTokenBuffer = memoryManager->getGlobalMemorySlot(CHANNEL_TAG, TOKEN_BUFFER_KEY);
 auto producerCoordinationBuffer = memoryManager->getGlobalMemorySlot(CHANNEL_TAG, PRODUCER_COORDINATION_BUFFER_KEY);

 // Creating producer and consumer channels
 auto consumer = HiCR::L1::channel::SPSC::Consumer(memoryManager, globalTokenBuffer, consumerCoordinationBuffer, producerCoordinationBuffer, sizeof(ELEMENT_TYPE), channelCapacity);

 // Getting a single value from the channel
 while (consumer.isEmpty()) consumer.updateDepth();

 // Getting internal pointer of the token buffer slot
 auto tokenBuffer = (ELEMENT_TYPE*)tokenBufferSlot->getPointer();

 printf("Received Value: %u\n", tokenBuffer[consumer.peek()]);
 consumer.pop();

 // Getting two values from the channel at once
 while (consumer.getDepth() < 2) consumer.updateDepth();
 printf("Received Value: %u\n", tokenBuffer[consumer.peek(0)]);
 printf("Received Value: %u\n", tokenBuffer[consumer.peek(1)]);
 consumer.pop(2);

 // Synchronizing so that all actors have finished registering their global memory slots
 memoryManager->fence(CHANNEL_TAG);

 // De-registering global slots
 memoryManager->deregisterGlobalMemorySlot(globalTokenBuffer);
 memoryManager->deregisterGlobalMemorySlot(producerCoordinationBuffer);

 // Freeing up local memory
 memoryManager->freeLocalMemorySlot(tokenBufferSlot);
}
