#pragma once

#include <hicr/channel/mpsc/consumer.hpp>
#include <common.hpp>

void consumerFc(HiCR::backend::MemoryManager* memoryManager, const size_t channelCapacity, const size_t producerCount)
{
 // Obtaining memory spaces
 auto memSpaces = memoryManager->getMemorySpaceList();

 // Getting required buffer sizes
 auto tokenBufferSize = HiCR::channel::Base::getTokenBufferSize(sizeof(ELEMENT_TYPE), channelCapacity);

 // Registering token buffer as a local memory slot
 auto tokenBufferSlot = memoryManager->allocateLocalMemorySlot(*memSpaces.begin(), tokenBufferSize);

 // Getting required buffer size
 auto coordinationBufferSize = HiCR::channel::Base::getCoordinationBufferSize();

 // Registering token buffer as a local memory slot
 auto consumerCoordinationBufferSlot = memoryManager->allocateLocalMemorySlot(*memSpaces.begin(), coordinationBufferSize);

 // Initializing coordination buffer (sets to zero the counters)
 HiCR::channel::Base::initializeCoordinationBuffer(consumerCoordinationBufferSlot);

 // Exchanging local memory slots to become global for them to be used by the remote end
 memoryManager->exchangeGlobalMemorySlots(CHANNEL_TAG, { { TOKEN_BUFFER_KEY, tokenBufferSlot }, { COORDINATION_BUFFER_KEY, consumerCoordinationBufferSlot } });

 // Synchronizing so that all actors have finished registering their global memory slots
 memoryManager->fence(CHANNEL_TAG);

 // Obtaining the globally exchanged memory slots
 auto globalTokenBufferSlot        = memoryManager->getGlobalMemorySlot(CHANNEL_TAG, TOKEN_BUFFER_KEY);
 auto globalCoordinationBufferSlot = memoryManager->getGlobalMemorySlot(CHANNEL_TAG, COORDINATION_BUFFER_KEY);

 // Creating producer and consumer channels
 auto consumer = HiCR::channel::MPSC::Consumer(memoryManager, globalTokenBufferSlot, globalCoordinationBufferSlot, sizeof(ELEMENT_TYPE), channelCapacity);

 // Getting internal pointer of the token buffer slot
 auto tokenBuffer = (ELEMENT_TYPE*)tokenBufferSlot->getPointer();

 // Calculating the expected message count
 size_t expectedMessageCount = MESSAGES_PER_PRODUCER * producerCount;
 size_t receivedMessageCount = 0;

 // Waiting for all messages to arrive, and printing them one by one
 while (receivedMessageCount < expectedMessageCount)
 {
  // Storage for the token position
  auto pos = consumer.peek();

  // Waiting for the next message
  while (pos < 0) pos = consumer.peek();

  // Increasing received message counter
  receivedMessageCount++;

  // Printing value
  printf("    [Consumer] Recv Value: %u  (%lu/%lu) Pos: %ld\n", tokenBuffer[pos], receivedMessageCount, expectedMessageCount, pos);

  // Disposing of printed value
  while (consumer.pop() == false);
 }

 // Synchronizing so that all actors have finished registering their global memory slots
 memoryManager->fence(CHANNEL_TAG);

 // De-registering global slots
 memoryManager->deregisterGlobalMemorySlot(globalTokenBufferSlot);
 memoryManager->deregisterGlobalMemorySlot(globalCoordinationBufferSlot);

 // Freeing up local memory
 memoryManager->freeLocalMemorySlot(tokenBufferSlot);
 memoryManager->freeLocalMemorySlot(consumerCoordinationBufferSlot);
}

