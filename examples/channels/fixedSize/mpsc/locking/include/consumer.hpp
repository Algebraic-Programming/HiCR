#pragma once

#include <hicr/core/L0/memorySpace.hpp>
#include <hicr/core/L1/memoryManager.hpp>
#include <hicr/core/L1/communicationManager.hpp>
#include <hicr/frontends/channel/fixedSize/mpsc/locking/consumer.hpp>
#include "common.hpp"

void consumerFc(HiCR::L1::MemoryManager               &memoryManager,
                HiCR::L1::CommunicationManager        &communicationManager,
                std::shared_ptr<HiCR::L0::MemorySpace> bufferMemorySpace,
                const size_t                           channelCapacity,
                const size_t                           producerCount)
{
  // Getting required buffer sizes
  auto tokenBufferSize = HiCR::channel::fixedSize::Base::getTokenBufferSize(sizeof(ELEMENT_TYPE), channelCapacity);

  // Registering token buffer as a local memory slot
  auto tokenBufferSlot = memoryManager.allocateLocalMemorySlot(bufferMemorySpace, tokenBufferSize);

  // Getting required buffer size
  auto coordinationBufferSize = HiCR::channel::fixedSize::Base::getCoordinationBufferSize();

  // Registering token buffer as a local memory slot
  auto coordinationBuffer = memoryManager.allocateLocalMemorySlot(bufferMemorySpace, coordinationBufferSize);

  // Initializing coordination buffer (sets to zero the counters)
  HiCR::channel::fixedSize::Base::initializeCoordinationBuffer(coordinationBuffer);

  // Exchanging local memory slots to become global for them to be used by the remote end
  communicationManager.exchangeGlobalMemorySlots(CHANNEL_TAG,                          /* global tag */
                                                 {{TOKEN_BUFFER_KEY, tokenBufferSlot}, /* key-slot pairs */
                                                  {CONSUMER_COORDINATION_BUFFER_KEY, coordinationBuffer}});

  // Synchronizing so that all actors have finished registering their global memory slots
  communicationManager.fence(CHANNEL_TAG);

  // Obtaining the globally exchanged memory slots
  auto globalTokenBufferSlot      = communicationManager.getGlobalMemorySlot(CHANNEL_TAG, TOKEN_BUFFER_KEY);
  auto consumerCoordinationBuffer = communicationManager.getGlobalMemorySlot(CHANNEL_TAG, CONSUMER_COORDINATION_BUFFER_KEY);

  // Creating producer and consumer channels
  auto consumer = HiCR::channel::fixedSize::MPSC::locking::Consumer(communicationManager,
                                                                    globalTokenBufferSlot, /* tokenBuffer */
                                                                    coordinationBuffer,    /* internalCoordinationBuffer */
                                                                    consumerCoordinationBuffer,
                                                                    sizeof(ELEMENT_TYPE),
                                                                    channelCapacity);

  // Getting internal pointer of the token buffer slot
  auto tokenBuffer = (ELEMENT_TYPE *)tokenBufferSlot->getPointer();

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
  communicationManager.fence(CHANNEL_TAG);

  // De-registering global slots
  communicationManager.deregisterGlobalMemorySlot(globalTokenBufferSlot);
  communicationManager.deregisterGlobalMemorySlot(consumerCoordinationBuffer);

  communicationManager.destroyGlobalMemorySlot(globalTokenBufferSlot);
  communicationManager.destroyGlobalMemorySlot(consumerCoordinationBuffer);

  communicationManager.fence(CHANNEL_TAG);

  // Freeing up local memory
  memoryManager.freeLocalMemorySlot(tokenBufferSlot);
  memoryManager.freeLocalMemorySlot(coordinationBuffer);
}
