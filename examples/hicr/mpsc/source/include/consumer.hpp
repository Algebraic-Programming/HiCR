#pragma once

#include <hicr/mpsc/consumer.hpp>
#include <common.hpp>

void consumerFc(HiCR::backend::MemoryManager* memoryManager, const size_t channelCapacity)
{
 // Obtaining memory spaces
 auto memSpaces = memoryManager->getMemorySpaceList();

 // Getting required buffer sizes
 auto tokenBufferSize = HiCR::MPSC::Base::getTokenBufferSize(sizeof(ELEMENT_TYPE), channelCapacity);

 // Registering token buffer as a local memory slot
 auto localTokenBufferSlot = memoryManager->allocateLocalMemorySlot(*memSpaces.begin(), tokenBufferSize);

 // Getting required buffer size
 auto coordinationBufferSize = HiCR::MPSC::Base::getCoordinationBufferSize();

 // Registering token buffer as a local memory slot
 auto localCoordinationBufferSlot = memoryManager->allocateLocalMemorySlot(*memSpaces.begin(), coordinationBufferSize);

 // Initializing coordination buffer (sets to zero the counters)
 HiCR::ProducerChannel::initializeCoordinationBuffer(localCoordinationBufferSlot);

 // Exchanging local memory slots to become global for them to be used by the remote end
 memoryManager->exchangeGlobalMemorySlots(CHANNEL_TAG, { { TOKEN_BUFFER_KEY, localTokenBufferSlot }, { COORDINATION_BUFFER_KEY, localCoordinationBufferSlot } });

 // Synchronizing so that all actors have finished registering their global memory slots
 memoryManager->fence(CHANNEL_TAG);

 // Obtaining the globally exchanged memory slots
 auto globalTokenBufferSlot        = memoryManager->getGlobalMemorySlot(CHANNEL_TAG, TOKEN_BUFFER_KEY);
 auto globalCoordinationBufferSlot = memoryManager->getGlobalMemorySlot(CHANNEL_TAG, COORDINATION_BUFFER_KEY);

 // Creating producer and consumer channels
 auto consumer = HiCR::MPSC::Consumer(memoryManager, globalTokenBufferSlot, localCoordinationBufferSlot, globalCoordinationBufferSlot, sizeof(ELEMENT_TYPE), channelCapacity);

 // Getting internal pointer of the token buffer slot
 auto tokenBuffer = (ELEMENT_TYPE*)localTokenBufferSlot->getPointer();

 // Getting a single value from the channel
 while (consumer.peek() < 0);
 printf("Received Value: %u\n", tokenBuffer[consumer.peek()]);
 while (consumer.pop() == false);

 // Getting two values from the channel at once
 while (consumer.peek(1) < 0);
 printf("Received Value: %u\n", tokenBuffer[consumer.peek(0)]);
 printf("Received Value: %u\n", tokenBuffer[consumer.peek(1)]);
 while (consumer.pop(2) == false);

 // Synchronizing so that all actors have finished registering their global memory slots
 memoryManager->fence(CHANNEL_TAG);

 // De-registering global slots
 memoryManager->deregisterGlobalMemorySlot(localTokenBufferSlot);
 memoryManager->deregisterGlobalMemorySlot(localCoordinationBufferSlot);

 // Freeing up local memory
 memoryManager->freeLocalMemorySlot(localTokenBufferSlot);
 memoryManager->freeLocalMemorySlot(localCoordinationBufferSlot);
}

