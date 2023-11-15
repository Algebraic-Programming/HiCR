#pragma once

#include <common.hpp>
#include <hicr.hpp>

void producerFc(HiCR::backend::MemoryManager *memoryManager, const size_t channelCapacity)
{
  // Getting required buffer size
  auto coordinationBufferSize = HiCR::ProducerChannel::getCoordinationBufferSize();

 // Obtaining memory spaces
 auto memSpaces = memoryManager->getMemorySpaceList();

 // Registering token buffer as a local memory slot
 auto coordinationBufferSlot = memoryManager->allocateLocalMemorySlot(*memSpaces.begin(), coordinationBufferSize);

  // Initializing coordination buffer (sets to zero the counters)
  HiCR::ProducerChannel::initializeCoordinationBuffer(coordinationBufferSlot);

  // Exchanging local memory slots to become global for them to be used by the remote end
  memoryManager->exchangeGlobalMemorySlots(CHANNEL_TAG, {{PRODUCER_KEY, coordinationBufferSlot}});

  // Synchronizing so that all actors have finished registering their global memory slots
  memoryManager->fence(CHANNEL_TAG);

  // Obtaining the globally exchanged memory slots
  auto consumerBuffer = memoryManager->getGlobalMemorySlot(CHANNEL_TAG, CONSUMER_KEY);
  auto producerBuffer = memoryManager->getGlobalMemorySlot(CHANNEL_TAG, PRODUCER_KEY);

  // Creating producer and consumer channels
  auto producer = HiCR::ProducerChannel(memoryManager, consumerBuffer, producerBuffer, sizeof(ELEMENT_TYPE), channelCapacity);

  // Allocating a send slot to put the values we want to communicate
  ELEMENT_TYPE sendBuffer = 0;
  auto sendBufferPtr = &sendBuffer;
  auto sendSlot = memoryManager->registerLocalMemorySlot(sendBufferPtr, sizeof(ELEMENT_TYPE));

  // Pushing values to the channel, one by one, suspending when/if the channel is full
  sendBuffer = 42;
  producer.push(sendSlot);
  printf("Sent Value:     %u\n", *sendBufferPtr);

  // If channel is full, wait until it frees up
  while (producer.isFull()) producer.updateDepth();

  // Sending second value
  sendBuffer = 43;
  producer.push(sendSlot);
  printf("Sent Value:     %u\n", *sendBufferPtr);

  // If channel is full, wait until it frees up
  while (producer.isFull()) producer.updateDepth();

  // Sending third value
  sendBuffer = 44;
  producer.push(sendSlot);
  printf("Sent Value:     %u\n", *sendBufferPtr);

  // Synchronizing so that all actors have finished registering their global memory slots
  memoryManager->fence(CHANNEL_TAG);

 // De-registering global slots
 memoryManager->deregisterGlobalMemorySlot(consumerBuffer);
 memoryManager->deregisterGlobalMemorySlot(producerBuffer);

 // Freeing up local memory
 memoryManager->freeLocalMemorySlot(coordinationBufferSlot);
}
