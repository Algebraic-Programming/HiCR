#pragma once

#include <hicr.hpp>
#include <source/common.hpp>

void consumerFc(HiCR::Backend* backend, const size_t channelCapacity)
{
 // Getting required buffer sizes
 auto tokenBufferSize = HiCR::ConsumerChannel::getTokenBufferSize(sizeof(ELEMENT_TYPE), channelCapacity);

 // Getting local pointer of token buffer
 auto tokenBuffer = (ELEMENT_TYPE*) malloc(tokenBufferSize);

 // Registering token buffer as MPI memory slot
 auto tokenBufferSlot = backend->registerLocalMemorySlot(tokenBuffer, tokenBufferSize);

 // Registering buffers globally for them to be used by remote actors
 backend->promoteMemorySlotToGlobal(CHANNEL_TAG, CONSUMER_KEY, tokenBufferSlot);

 // Synchronizing so that all actors have finished registering their global memory slots
 backend->fence(CHANNEL_TAG);

 // Obtaining the globally exchanged memory slots
 auto consumerBuffer = backend->getGlobalMemorySlots(CHANNEL_TAG, CONSUMER_KEY)[0];
 auto producerBuffer = backend->getGlobalMemorySlots(CHANNEL_TAG, PRODUCER_KEY)[0];

 // Creating producer and consumer channels
 auto consumer = HiCR::ConsumerChannel(backend, consumerBuffer, producerBuffer, sizeof(ELEMENT_TYPE), channelCapacity);

 // Getting a single value from the channel
 while (consumer.isEmpty());
 printf("Received Value: %u\n", tokenBuffer[consumer.peek()]);
 consumer.pop();

 // Getting two values from the channel at once
 while (consumer.queryDepth() < 2);
 printf("Received Value: %u\n", tokenBuffer[consumer.peek(0)]);
 printf("Received Value: %u\n", tokenBuffer[consumer.peek(1)]);
 consumer.pop(2);

 // Synchronizing before deleting the channel and freeing up memory
 backend->fence(CHANNEL_TAG);

 // De-registering local slots
 backend->deregisterLocalMemorySlot(tokenBufferSlot);
 backend->deregisterGlobalMemorySlot(consumerBuffer);
 backend->deregisterGlobalMemorySlot(producerBuffer);

 // Freeing up memory
 free(tokenBuffer);
}

