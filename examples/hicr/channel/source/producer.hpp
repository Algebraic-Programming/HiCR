#pragma once

#include <hicr.hpp>
#include <source/common.hpp>

void producerFc(HiCR::Backend* backend, const size_t channelCapacity)
{
 // Getting required buffer size
 auto coordinationBufferSize = HiCR::ProducerChannel::getCoordinationBufferSize();

 // Getting local allocation of coordination buffer
 auto coordinationBuffer = malloc(coordinationBufferSize);

 // Registering coordination buffer as a local memory slot
 auto coordinationBufferSlot = backend->registerLocalMemorySlot(coordinationBuffer, coordinationBufferSize);

 // Initializing coordination buffer (sets to zero the counters)
 HiCR::ProducerChannel::initializeCoordinationBuffer(backend, coordinationBufferSlot);

 // Promoting local memory slots to global for them to be used by the remote end
 backend->promoteMemorySlotToGlobal(CHANNEL_TAG, PRODUCER_KEY, coordinationBufferSlot);

 // Synchronizing so that all actors have finished registering their global memory slots
 backend->fence(CHANNEL_TAG);

 // Obtaining the globally exchanged memory slots
 auto consumerBuffer = backend->getGlobalMemorySlots(CHANNEL_TAG, CONSUMER_KEY)[0];
 auto producerBuffer = backend->getGlobalMemorySlots(CHANNEL_TAG, PRODUCER_KEY)[0];

 // Creating producer and consumer channels
 auto producer = HiCR::ProducerChannel(backend, consumerBuffer, producerBuffer, sizeof(ELEMENT_TYPE), channelCapacity);

 // Allocating a send slot to put the values we want to communicate
 ELEMENT_TYPE sendBuffer = 0;
 auto sendBufferPtr = &sendBuffer;
 auto sendSlot = backend->registerLocalMemorySlot(sendBufferPtr, sizeof(ELEMENT_TYPE));

 // Pushing values to the channel, one by one, suspending when/if the channel is full
 sendBuffer = 42;
 producer.push(sendSlot);
 printf("Sent Value:     %u\n", *sendBufferPtr);

 // If channel is full, wait until it frees up
 while(producer.isFull());

 // Sending second value
 sendBuffer = 43;
 producer.push(sendSlot);
 printf("Sent Value:     %u\n", *sendBufferPtr);

 // If channel is full, wait until it frees up
 while(producer.isFull());

 // Sending third value
 sendBuffer = 44;
 producer.push(sendSlot);
 printf("Sent Value:     %u\n", *sendBufferPtr);

 // Synchronizing before deleting the channel and freeing up memory
 backend->fence(CHANNEL_TAG);

 // De-registering local slots
 backend->deregisterLocalMemorySlot(coordinationBufferSlot);
 backend->deregisterGlobalMemorySlot(consumerBuffer);
 backend->deregisterGlobalMemorySlot(producerBuffer);

 // Freeing up memory
 free(coordinationBuffer);
}


