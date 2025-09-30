#pragma once

#include "common.hpp"
#include <hicr/core/memoryManager.hpp>
#include <hicr/core/communicationManager.hpp>
#include <hicr/frontends/channel/fixedSize/spsc/producer.hpp>
#include <hicr/frontends/channel/fixedSize/spsc/consumer.hpp>

#include <iostream>

void producerFc(HiCR::MemoryManager               &memoryManager,
                HiCR::CommunicationManager        &communicationManager,
                std::shared_ptr<HiCR::MemorySpace> bufferMemorySpace,
                const size_t                       channelCapacity,
                const int                          msgCount,
                const int                          tokenSize)
{
  // Getting required buffer sizes
  auto tokenBufferSize = HiCR::channel::fixedSize::Base::getTokenBufferSize(tokenSize, channelCapacity);

  // Allocating token buffer as a local memory slot
  auto pongBufferSlot = memoryManager.allocateLocalMemorySlot(bufferMemorySpace, tokenBufferSize);

  // Getting required buffer size
  auto coordinationBufferSize = HiCR::channel::fixedSize::Base::getCoordinationBufferSize();

  // Allocating coordination buffer as a local memory slot
  auto pingCoordinationBuffer = memoryManager.allocateLocalMemorySlot(bufferMemorySpace, coordinationBufferSize);
  auto pongCoordinationBuffer = memoryManager.allocateLocalMemorySlot(bufferMemorySpace, coordinationBufferSize);

  // Initializing coordination buffer (sets to zero the counters)
  HiCR::channel::fixedSize::Base::initializeCoordinationBuffer(pingCoordinationBuffer);
  HiCR::channel::fixedSize::Base::initializeCoordinationBuffer(pongCoordinationBuffer);

  // Exchanging local memory slots to become global for them to be used by the remote end
  communicationManager.exchangeGlobalMemorySlots(
    CHANNEL_TAG,
    {{PRODUCER_PING_COORDINATION_BUFFER_KEY, pingCoordinationBuffer}, {PRODUCER_PONG_COORDINATION_BUFFER_KEY, pongCoordinationBuffer}, {PONG_BUFFER_KEY, pongBufferSlot}});

  // Synchronizing so that all actors have finished registering their global memory slots
  communicationManager.fence(CHANNEL_TAG);

  // Obtaining the globally exchanged memory slots
  auto pingTokenBufferSlot            = communicationManager.getGlobalMemorySlot(CHANNEL_TAG, PING_BUFFER_KEY);
  auto pongTokenBufferSlot            = communicationManager.getGlobalMemorySlot(CHANNEL_TAG, PONG_BUFFER_KEY);
  auto producerPingCoordinationBuffer = communicationManager.getGlobalMemorySlot(CHANNEL_TAG, PRODUCER_PING_COORDINATION_BUFFER_KEY);
  auto producerPongCoordinationBuffer = communicationManager.getGlobalMemorySlot(CHANNEL_TAG, PRODUCER_PONG_COORDINATION_BUFFER_KEY);
  auto consumerPingCoordinationBuffer = communicationManager.getGlobalMemorySlot(CHANNEL_TAG, CONSUMER_PING_COORDINATION_BUFFER_KEY);
  auto consumerPongCoordinationBuffer = communicationManager.getGlobalMemorySlot(CHANNEL_TAG, CONSUMER_PONG_COORDINATION_BUFFER_KEY);

  // Creating producer and consumer channels
  auto pingChannel = HiCR::channel::fixedSize::SPSC::Producer(
    communicationManager, communicationManager, pingTokenBufferSlot, pingCoordinationBuffer, consumerPingCoordinationBuffer, tokenSize, channelCapacity);
  // For the ponger, the consumer buffer i
  auto pongChannel = HiCR::channel::fixedSize::SPSC::Consumer(
    communicationManager, communicationManager, pongTokenBufferSlot, pongCoordinationBuffer, consumerPongCoordinationBuffer, tokenSize, channelCapacity);

  // Allocating a send slot to put the values we want to communicate
  ELEMENT_TYPE sendBuffer[tokenSize] = {'a'};
  auto         sendBufferPtr         = sendBuffer;
  auto         sendSlot              = memoryManager.registerLocalMemorySlot(bufferMemorySpace, sendBufferPtr, tokenSize);

  // Pushing values to the channel, one by one, suspending when/if the channel is full
  for (int i = 0; i < msgCount; i++)
  {
    while (pingChannel.isFull()) pingChannel.updateDepth();
    pingChannel.push(sendSlot);

    while (pongChannel.isEmpty()) pongChannel.updateDepth();
    pongChannel.pop();
  }

  // Synchronizing so that all actors have finished registering their global memory slots
  communicationManager.fence(CHANNEL_TAG);

  // De-registering global slots (collective calls)
  communicationManager.deregisterGlobalMemorySlot(pingTokenBufferSlot);
  communicationManager.deregisterGlobalMemorySlot(pongTokenBufferSlot);
  communicationManager.deregisterGlobalMemorySlot(consumerPingCoordinationBuffer);
  communicationManager.deregisterGlobalMemorySlot(consumerPongCoordinationBuffer);
  communicationManager.deregisterGlobalMemorySlot(producerPingCoordinationBuffer);
  communicationManager.deregisterGlobalMemorySlot(producerPongCoordinationBuffer);

  // Freeing up local memory
  memoryManager.freeLocalMemorySlot(pingCoordinationBuffer);
  memoryManager.freeLocalMemorySlot(pongCoordinationBuffer);
  memoryManager.freeLocalMemorySlot(pongBufferSlot);
}
