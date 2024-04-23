#pragma once

#include <hicr/core/L0/memorySpace.hpp>
#include <hicr/core/L1/memoryManager.hpp>
#include <hicr/core/L1/communicationManager.hpp>
#include <hicr/frontends/channel/fixedSize/mpsc/nonlocking/producer.hpp>
#include "common.hpp"
#include <iostream>

void producerFc(HiCR::L1::MemoryManager               &memoryManager,
                HiCR::L1::CommunicationManager        &communicationManager,
                std::shared_ptr<HiCR::L0::MemorySpace> bufferMemorySpace,
                const size_t                           channelCapacity,
                const size_t                           producerId,
                const size_t                           producerCount)
{
  // initialize producer coordination buffer
  auto coordinationBufferSize = HiCR::channel::fixedSize::Base::getCoordinationBufferSize();
  auto coordinationBuffer     = memoryManager.allocateLocalMemorySlot(bufferMemorySpace, coordinationBufferSize);
  HiCR::channel::fixedSize::Base::initializeCoordinationBuffer(coordinationBuffer);

  // these vectors hold all producer buffers and are only used for later deregistration
  std::vector<std::shared_ptr<HiCR::L0::GlobalMemorySlot>> producerCoordinationBuffers;
  std::vector<std::shared_ptr<HiCR::L0::GlobalMemorySlot>> globalTokenBuffers;

  // get from consumer all the token buffer information
  communicationManager.exchangeGlobalMemorySlots(TOKEN_TAG, {});
  communicationManager.fence(TOKEN_TAG);
  // communicate to consumer all producer coordination buffers
  communicationManager.exchangeGlobalMemorySlots(PRODUCER_COORDINATION_TAG, {{producerId, coordinationBuffer}});
  communicationManager.fence(PRODUCER_COORDINATION_TAG);

  for (size_t i = 0; i < producerCount; i++)
  {
    auto globalTokenBufferSlot = communicationManager.getGlobalMemorySlot(TOKEN_TAG, i);
    globalTokenBuffers.push_back(globalTokenBufferSlot);
    auto producerCoordinationBuffer = communicationManager.getGlobalMemorySlot(PRODUCER_COORDINATION_TAG, i);
    producerCoordinationBuffers.push_back(producerCoordinationBuffer);
  }

  // Creating producer channel
  // This call does the same as the SPSC Producer constructor, as
  // the producer of MPSC::nonlocking has the same view
  auto producer = HiCR::channel::fixedSize::MPSC::nonlocking::Producer(
    communicationManager, globalTokenBuffers[producerId], coordinationBuffer, producerCoordinationBuffers[producerId], sizeof(ELEMENT_TYPE), channelCapacity);

  // Allocating a send slot to put the values we want to communicate
  ELEMENT_TYPE sendBuffer    = 0;
  auto         sendBufferPtr = &sendBuffer;
  auto         sendSlot      = memoryManager.registerLocalMemorySlot(bufferMemorySpace, sendBufferPtr, sizeof(ELEMENT_TYPE));

  // Pushing values to the channel, one by one, suspending when/if the channel is full
  for (size_t i = 0; i < MESSAGES_PER_PRODUCER; i++)
  {
    // Calculating message value
    sendBuffer = 42 + i;

    // If channel is full, wait until it frees up - needed for nonlocking MPSC
    while (producer.isFull()) producer.updateDepth();

    // This push expects to always succeed (unlike the locking MPSC push)
    producer.push(sendSlot);
    // Printing value sent
    printf("[Producer %03lu] Sent Value: %u\n", producerId, *sendBufferPtr);
  }

  communicationManager.fence(TOKEN_TAG);
  communicationManager.fence(PRODUCER_COORDINATION_TAG);

  for (size_t i = 0; i < producerCount; i++)
  {
    communicationManager.deregisterGlobalMemorySlot(globalTokenBuffers[i]);
    communicationManager.deregisterGlobalMemorySlot(producerCoordinationBuffers[i]);
  }

  memoryManager.freeLocalMemorySlot(coordinationBuffer);
}
