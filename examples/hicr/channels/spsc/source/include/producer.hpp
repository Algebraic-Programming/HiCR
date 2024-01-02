#pragma once

#include "common.hpp"
#include <hicr/L1/memoryManager.hpp>
#include <hicr/L1/communicationManager.hpp>
#include <hicr/L2/channel/spsc/producer.hpp>

void producerFc(HiCR::L1::MemoryManager *memoryManager, HiCR::L1::CommunicationManager *communicationManager, HiCR::L0::MemorySpace* bufferMemorySpace, const size_t channelCapacity)
{
  // Getting required buffer size
  auto coordinationBufferSize = HiCR::L2::channel::Base::getCoordinationBufferSize();

  // Allocating token buffer as a local memory slot
  auto coordinationBuffer = memoryManager->allocateLocalMemorySlot(bufferMemorySpace, coordinationBufferSize);

  // Initializing coordination buffer (sets to zero the counters)
  HiCR::L2::channel::Base::initializeCoordinationBuffer(coordinationBuffer);

  // Exchanging local memory slots to become global for them to be used by the remote end
  communicationManager->exchangeGlobalMemorySlots(CHANNEL_TAG, {{PRODUCER_COORDINATION_BUFFER_KEY, coordinationBuffer}});

  // Synchronizing so that all actors have finished registering their global memory slots
  communicationManager->fence(CHANNEL_TAG);

  // Obtaining the globally exchanged memory slots
  auto tokenBuffer = communicationManager->getGlobalMemorySlot(CHANNEL_TAG, TOKEN_BUFFER_KEY);
  auto producerCoordinationBuffer = communicationManager->getGlobalMemorySlot(CHANNEL_TAG, PRODUCER_COORDINATION_BUFFER_KEY);

  // Creating producer and consumer channels
  auto producer = HiCR::L2::channel::SPSC::Producer(communicationManager, tokenBuffer, coordinationBuffer, producerCoordinationBuffer, sizeof(ELEMENT_TYPE), channelCapacity);

  // Allocating a send slot to put the values we want to communicate
  ELEMENT_TYPE sendBuffer = 0;
  auto sendBufferPtr = &sendBuffer;
  auto sendSlot = memoryManager->registerLocalMemorySlot(bufferMemorySpace, sendBufferPtr, sizeof(ELEMENT_TYPE));

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
  communicationManager->fence(CHANNEL_TAG);

  // De-registering global slots (collective calls)
  communicationManager->deregisterGlobalMemorySlot(tokenBuffer);
  communicationManager->deregisterGlobalMemorySlot(producerCoordinationBuffer);

  // Freeing up local memory
  memoryManager->freeLocalMemorySlot(coordinationBuffer);
}
