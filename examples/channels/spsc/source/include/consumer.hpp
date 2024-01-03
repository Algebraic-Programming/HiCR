#pragma once

#include "common.hpp"
#include <hicr/L1/memoryManager.hpp>
#include <hicr/L1/communicationManager.hpp>
#include <frontends/channel/spsc/consumer.hpp>

void consumerFc(HiCR::L1::MemoryManager *memoryManager, HiCR::L1::CommunicationManager *communicationManager, HiCR::L0::MemorySpace* bufferMemorySpace, const size_t channelCapacity)
{
  // Getting required buffer sizes
  auto tokenBufferSize = HiCR::channel::Base::getTokenBufferSize(sizeof(ELEMENT_TYPE), channelCapacity);

  // Allocating token buffer as a local memory slot
  auto tokenBufferSlot = memoryManager->allocateLocalMemorySlot(bufferMemorySpace, tokenBufferSize);

  // Getting required buffer size
  auto coordinationBufferSize = HiCR::channel::Base::getCoordinationBufferSize();

  // Allocating coordination buffer as a local memory slot
  auto coordinationBuffer = memoryManager->allocateLocalMemorySlot(bufferMemorySpace, coordinationBufferSize);

  // Initializing coordination buffer (sets to zero the counters)
  HiCR::channel::Base::initializeCoordinationBuffer(coordinationBuffer); 

  // Exchanging local memory slots to become global for them to be used by the remote end
  communicationManager->exchangeGlobalMemorySlots(CHANNEL_TAG, {{TOKEN_BUFFER_KEY, tokenBufferSlot}, {CONSUMER_COORDINATION_BUFFER_KEY, coordinationBuffer}});

  // Synchronizing so that all actors have finished registering their global memory slots
  communicationManager->fence(CHANNEL_TAG);

  // Obtaining the globally exchanged memory slots
  auto globalTokenBufferSlot      = communicationManager->getGlobalMemorySlot(CHANNEL_TAG, TOKEN_BUFFER_KEY);
  auto producerCoordinationBuffer = communicationManager->getGlobalMemorySlot(CHANNEL_TAG, PRODUCER_COORDINATION_BUFFER_KEY);

  // Creating producer and consumer channels
  auto consumer = HiCR::channel::SPSC::Consumer(communicationManager, globalTokenBufferSlot, coordinationBuffer, producerCoordinationBuffer, sizeof(ELEMENT_TYPE), channelCapacity);

  // Getting a single value from the channel
  while (consumer.isEmpty()) consumer.updateDepth();

  // Getting internal pointer of the token buffer slot
  auto tokenBuffer = (ELEMENT_TYPE *)tokenBufferSlot->getPointer();

  printf("Received Value: %u\n", tokenBuffer[consumer.peek()]);
  consumer.pop();

  // Getting two values from the channel at once
  while (consumer.getDepth() < 2) consumer.updateDepth();
  printf("Received Value: %u\n", tokenBuffer[consumer.peek(0)]);
  printf("Received Value: %u\n", tokenBuffer[consumer.peek(1)]);
  consumer.pop(2);

  // Synchronizing so that all actors have finished registering their global memory slots
  communicationManager->fence(CHANNEL_TAG);

  // De-registering global slots (collective calls)
  communicationManager->deregisterGlobalMemorySlot(globalTokenBufferSlot);
  communicationManager->deregisterGlobalMemorySlot(producerCoordinationBuffer);

  // Freeing up local memory
  memoryManager->freeLocalMemorySlot(tokenBufferSlot);
  memoryManager->freeLocalMemorySlot(coordinationBuffer);
}
