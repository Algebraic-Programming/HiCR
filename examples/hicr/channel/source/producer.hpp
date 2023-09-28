#pragma once

#include <hicr.hpp>
#include <source/common.hpp>

void producerFc(HiCR::Backend* backend)
{
 // Getting required buffer size
 auto coordinationBufferSize = HiCR::ProducerChannel::getCoordinationBufferSize();

 // Getting local allocation of coordination buffer
 auto coordinationBuffer = malloc(coordinationBufferSize);

 // Registering coordination buffer as MPI memory slot
 auto coordinationBufferSlot = backend->registerLocalMemorySlot(coordinationBuffer, coordinationBufferSize);

 // Initializing coordination buffer (sets to zero the counters)
 HiCR::ProducerChannel::initializeCoordinationBuffer(backend, coordinationBufferSlot);

 // Registering buffers globally for them to be used by remote actors
 backend->exchangeGlobalMemorySlots(CHANNEL_TAG, PRODUCER_KEY, {coordinationBufferSlot});

 // Synchronizing so that all actors have finished registering their memory slots
 backend->fence(CHANNEL_TAG);

 // Obtaining the globally exchanged memory slots
 auto globalBuffers = backend->getGlobalMemorySlots()[CHANNEL_TAG];

 // Creating producer and consumer channels
 auto producer = HiCR::ProducerChannel(backend, globalBuffers[CONSUMER_KEY][0], globalBuffers[PRODUCER_KEY][0], sizeof(ELEMENT_TYPE), CAPACITY);

 // Allocating a send slot to put the values we want to communicate
 ELEMENT_TYPE sendBuffer = 0;
 auto sendBufferPtr = &sendBuffer;
 auto sendSlot = backend->registerLocalMemorySlot(sendBufferPtr, sizeof(ELEMENT_TYPE));

 // Pushing values to the channel, one by one, suspending when/if the channel is full
 sendBuffer = 42;
 while(producer.push(sendSlot) == false);
 printf("Sent Value:     %u\n", *sendBufferPtr);

 sendBuffer = 43;
 while(producer.push(sendSlot) == false);
 printf("Sent Value:     %u\n", *sendBufferPtr);

 sendBuffer = 44;
 while(producer.push(sendSlot) == false);
 printf("Sent Value:     %u\n", *sendBufferPtr);

 // Synchronizing before deleting the channel and freeing up memory
 backend->fence(CHANNEL_TAG);

 // De-registering local slots
 backend->deregisterLocalMemorySlot(coordinationBufferSlot);

 // Freeing up memory
 free(coordinationBuffer);
}


