#pragma once

#include <hicr.hpp>
#include <source/include/common.hpp>

void consumerFc(HiCR::backend::MemoryManager* memoryManager, const size_t channelCapacity)
{
 // Getting required buffer sizes
 auto tokenBufferSize = HiCR::ConsumerChannel::getTokenBufferSize(sizeof(ELEMENT_TYPE), channelCapacity);

 // Getting local pointer of token buffer
 auto tokenBuffer = (ELEMENT_TYPE*) malloc(tokenBufferSize);

 // Registering token buffer as a local memory slot
 auto tokenBufferSlot = memoryManager->registerLocalMemorySlot(tokenBuffer, tokenBufferSize);

 // Exchanging local memory slots to become global for them to be used by the remote end
 memoryManager->exchangeGlobalMemorySlots(CHANNEL_TAG, { { CONSUMER_KEY, tokenBufferSlot } });

 // Synchronizing so that all actors have finished registering their global memory slots
 memoryManager->fence(CHANNEL_TAG);

 // Obtaining the globally exchanged memory slots
 auto consumerBuffer = memoryManager->getGlobalMemorySlot(CHANNEL_TAG, CONSUMER_KEY);
 auto producerBuffer = memoryManager->getGlobalMemorySlot(CHANNEL_TAG, PRODUCER_KEY);

 // Creating producer and consumer channels
 auto consumer = HiCR::ConsumerChannel(memoryManager, consumerBuffer, producerBuffer, sizeof(ELEMENT_TYPE), channelCapacity);

 // Getting a single value from the channel
 while (consumer.isEmpty());
 printf("Received Value: %u\n", tokenBuffer[consumer.peek()]);
 consumer.pop();

 // Getting two values from the channel at once
 while (consumer.queryDepth() < 2);
 printf("Received Value: %u\n", tokenBuffer[consumer.peek(0)]);
 printf("Received Value: %u\n", tokenBuffer[consumer.peek(1)]);
 consumer.pop(2);

 // Synchronizing so that all actors have finished registering their global memory slots
 memoryManager->fence(CHANNEL_TAG);

 // De-registering local slots
 memoryManager->deregisterLocalMemorySlot(tokenBufferSlot);
 memoryManager->deregisterGlobalMemorySlot(consumerBuffer);
 memoryManager->deregisterGlobalMemorySlot(producerBuffer);

 // Freeing up memory
 free(tokenBuffer);
}

