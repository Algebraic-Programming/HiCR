/*
 *   Copyright 2025 Huawei Technologies Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <hicr/core/memorySpace.hpp>
#include <hicr/core/memoryManager.hpp>
#include <hicr/core/communicationManager.hpp>
#include <hicr/frontends/channel/fixedSize/mpsc/nonlocking/producer.hpp>
#include "common.hpp"
#include <iostream>

void producerFc(HiCR::MemoryManager               &memoryManager,
                HiCR::CommunicationManager        &communicationManager,
                std::shared_ptr<HiCR::MemorySpace> bufferMemorySpace,
                const size_t                       channelCapacity,
                const size_t                       producerId,
                const size_t                       producerCount)
{
  // initialize producer coordination buffer
  auto coordinationBufferSize = HiCR::channel::fixedSize::Base::getCoordinationBufferSize();
  auto coordinationBuffer     = memoryManager.allocateLocalMemorySlot(bufferMemorySpace, coordinationBufferSize);
  HiCR::channel::fixedSize::Base::initializeCoordinationBuffer(coordinationBuffer);

  // these vectors hold all producer buffers and are only used for later deregistration
  std::vector<std::shared_ptr<HiCR::GlobalMemorySlot>> globalTokenBuffers;

  // get from consumer all the token buffer information
  communicationManager.exchangeGlobalMemorySlots(TOKEN_TAG, {});
  communicationManager.fence(TOKEN_TAG);
  // communicate to consumer all producer coordination buffers
  communicationManager.exchangeGlobalMemorySlots(PRODUCER_COORDINATION_TAG, {{producerId, coordinationBuffer}});
  communicationManager.fence(PRODUCER_COORDINATION_TAG);

  communicationManager.exchangeGlobalMemorySlots(CONSUMER_COORDINATION_TAG, {});
  communicationManager.fence(CONSUMER_COORDINATION_TAG);

  for (size_t i = 0; i < producerCount; i++)
  {
    auto globalTokenBufferSlot = communicationManager.getGlobalMemorySlot(TOKEN_TAG, i);
    globalTokenBuffers.push_back(globalTokenBufferSlot);
  }
  auto consumerCoordinationBuffer = communicationManager.getGlobalMemorySlot(CONSUMER_COORDINATION_TAG, producerId);
  // Creating producer channel
  // This call does the same as the SPSC Producer constructor, as
  // the producer of MPSC::nonlocking has the same view
  auto producer = HiCR::channel::fixedSize::MPSC::nonlocking::Producer(
    communicationManager, globalTokenBuffers[producerId], coordinationBuffer, consumerCoordinationBuffer, sizeof(ELEMENT_TYPE), channelCapacity);

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
  communicationManager.fence(CONSUMER_COORDINATION_TAG);

  // Clean-up on the consumer side, fence for slot destruction
  communicationManager.fence(TOKEN_TAG);
  communicationManager.fence(PRODUCER_COORDINATION_TAG);
  communicationManager.fence(CONSUMER_COORDINATION_TAG);

  memoryManager.freeLocalMemorySlot(coordinationBuffer);
}
