#pragma once

#include "common.hpp"
#include <hicr/L2/channel/mpsc/producer.hpp>

void producerFc(HiCR::L1::MemoryManager *memoryManager, HiCR::L0::MemorySpace* bufferMemorySpace, const size_t channelCapacity, const size_t producerId)
{
  // Getting required buffer size
  auto coordinationBufferSize = HiCR::L2::channel::Base::getCoordinationBufferSize();

  // Registering token buffer as a local memory slot
  auto producerCoordinationBufferSlot = memoryManager->allocateLocalMemorySlot(bufferMemorySpace, coordinationBufferSize);

  // Initializing coordination buffer (sets to zero the counters)
  HiCR::L2::channel::Base::initializeCoordinationBuffer(producerCoordinationBufferSlot);

  // Exchanging local memory slots to become global for them to be used by the remote end
  memoryManager->exchangeGlobalMemorySlots(CHANNEL_TAG, { {PRODUCER_COORDINATION_BUFFER_KEY + producerId, producerCoordinationBufferSlot} });

  // Synchronizing so that all actors have finished registering their global memory slots
  memoryManager->fence(CHANNEL_TAG);

  // Obtaining the globally exchanged memory slots
  auto globalTokenBuffer = memoryManager->getGlobalMemorySlot(CHANNEL_TAG, TOKEN_BUFFER_KEY);
  auto globalConsumerCoordinationBuffer = memoryManager->getGlobalMemorySlot(CHANNEL_TAG, CONSUMER_COORDINATION_BUFFER_KEY);
  auto globalProducerCoordinationBuffer = memoryManager->getGlobalMemorySlot(CHANNEL_TAG, PRODUCER_COORDINATION_BUFFER_KEY + producerId);

  // Creating producer and consumer channels
  auto producer = HiCR::L2::channel::MPSC::Producer(memoryManager, globalTokenBuffer, globalConsumerCoordinationBuffer, globalProducerCoordinationBuffer, sizeof(ELEMENT_TYPE), channelCapacity);

  // Allocating a send slot to put the values we want to communicate
  ELEMENT_TYPE sendBuffer = 0;
  auto sendBufferPtr = &sendBuffer;
  auto sendSlot = memoryManager->registerLocalMemorySlot(bufferMemorySpace, sendBufferPtr, sizeof(ELEMENT_TYPE));

  // Pushing values to the channel, one by one, suspending when/if the channel is full
  for (size_t i = 0; i < MESSAGES_PER_PRODUCER; i++)
  {
    // Calculating message value
    sendBuffer = 42 + i;

    // Trying to push (if the consumer buffer is busy or full, it will fail and try again)
    while (producer.push(sendSlot) == false)
      ;

    // Printing value sent
    printf("[Producer %03lu] Sent Value: %u\n", producerId, *sendBufferPtr);
  }

  // Synchronizing so that all actors have finished registering their global memory slots
  memoryManager->fence(CHANNEL_TAG);

  // De-registering global slots
  memoryManager->deregisterGlobalMemorySlot(globalTokenBuffer);
  memoryManager->deregisterGlobalMemorySlot(globalConsumerCoordinationBuffer);
  memoryManager->deregisterGlobalMemorySlot(globalProducerCoordinationBuffer);

  // Freeing up local memory
  memoryManager->freeLocalMemorySlot(producerCoordinationBufferSlot);
}
