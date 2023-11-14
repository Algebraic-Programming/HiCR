#pragma once

#include <hicr/mpsc/producer.hpp>
#include <common.hpp>

void producerFc(HiCR::backend::MemoryManager* memoryManager, const size_t channelCapacity)
{
 // Obtaining memory spaces
 auto memSpaces = memoryManager->getMemorySpaceList();

 // Getting required buffer size
 auto coordinationBufferSize = HiCR::MPSC::Base::getCoordinationBufferSize();

 // Registering token buffer as a local memory slot
 auto localCoordinationBufferSlot = memoryManager->allocateLocalMemorySlot(*memSpaces.begin(), coordinationBufferSize);

 // Exchanging local memory slots to become global for them to be used by the remote end
 memoryManager->exchangeGlobalMemorySlots(CHANNEL_TAG, { });

 // Synchronizing so that all actors have finished registering their global memory slots
 memoryManager->fence(CHANNEL_TAG);

 // Obtaining the globally exchanged memory slots
 auto globalTokenBufferSlot        = memoryManager->getGlobalMemorySlot(CHANNEL_TAG, TOKEN_BUFFER_KEY);
 auto globalCoordinationBufferSlot = memoryManager->getGlobalMemorySlot(CHANNEL_TAG, COORDINATION_BUFFER_KEY);

 // Creating producer and consumer channels
 auto producer = HiCR::MPSC::Producer(memoryManager, globalTokenBufferSlot, localCoordinationBufferSlot, globalCoordinationBufferSlot, sizeof(ELEMENT_TYPE), channelCapacity);

 // Allocating a send slot to put the values we want to communicate
 ELEMENT_TYPE sendBuffer = 0;
 auto sendBufferPtr = &sendBuffer;
 auto sendSlot = memoryManager->registerLocalMemorySlot(sendBufferPtr, sizeof(ELEMENT_TYPE));

 // Pushing values to the channel, one by one, suspending when/if the channel is full
 sendBuffer = 42;
 while (producer.push(sendSlot) == false);
 printf("Sent Value:     %u\n", *sendBufferPtr);

 // Sending second value
 sendBuffer = 43;
 while (producer.push(sendSlot) == false);
 printf("Sent Value:     %u\n", *sendBufferPtr);

 // Sending third value
 sendBuffer = 44;
 while (producer.push(sendSlot) == false);
 printf("Sent Value:     %u\n", *sendBufferPtr);

 // Synchronizing so that all actors have finished registering their global memory slots
 memoryManager->fence(CHANNEL_TAG);

 // De-registering global slots
 memoryManager->deregisterGlobalMemorySlot(globalTokenBufferSlot);
 memoryManager->deregisterGlobalMemorySlot(globalCoordinationBufferSlot);
}


