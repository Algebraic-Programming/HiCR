#pragma once

#include <hicr.hpp>
#include <source/common.hpp>

void consumerFc(HiCR::Backend* backend)
{
 // Getting required buffer sizes
 auto tokenBufferSize = HiCR::ConsumerChannel::getTokenBufferSize(sizeof(ELEMENT_TYPE), CAPACITY);

 // Getting local pointer of token buffer
 auto tokenBuffer = (ELEMENT_TYPE*) malloc(tokenBufferSize);

 // Registering token buffer as MPI memory slot
 auto tokenBufferSlot = backend->registerLocalMemorySlot(tokenBuffer, tokenBufferSize);

 // Registering buffers globally for them to be used by remote actors
 backend->exchangeGlobalMemorySlots(CHANNEL_TAG, CONSUMER_KEY, {tokenBufferSlot});

 // Synchronizing so that all actors have finished registering their memory slots
 backend->fence(CHANNEL_TAG);

 // Obtaining the globally exchanged memory slots
 auto globalBuffers = backend->getGlobalMemorySlots()[CHANNEL_TAG];

 // Creating producer and consumer channels
 auto consumer = HiCR::ConsumerChannel(backend, globalBuffers[CONSUMER_KEY][0], globalBuffers[PRODUCER_KEY][0], sizeof(ELEMENT_TYPE), CAPACITY);

 // Getting a single value from the channel
 auto recvSlot = consumer.peekWait();
 printf("Received Value: %u\n", tokenBuffer[recvSlot[0]]);
 consumer.pop();

 // Getting two values from the channel at once
 recvSlot = consumer.peekWait(2);
 printf("Received Value: %u\n", tokenBuffer[recvSlot[0]]);
 printf("Received Value: %u\n", tokenBuffer[recvSlot[1]]);
 consumer.pop(2);

 // Synchronizing before deleting the channel and freeing up memory
 backend->fence(CHANNEL_TAG);

 // De-registering local slots
 backend->deregisterLocalMemorySlot(tokenBufferSlot);

 // Freeing up memory
 free(tokenBuffer);
}

