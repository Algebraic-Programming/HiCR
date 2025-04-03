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

#include <hicr/core/L0/memorySpace.hpp>
#include <hicr/core/L1/memoryManager.hpp>
#include <hicr/core/L1/communicationManager.hpp>
#include <hicr/core/L0/globalMemorySlot.hpp>
#include <hicr/frontends/channel/fixedSize/mpsc/nonlocking/consumer.hpp>
#include "common.hpp"

void consumerFc(HiCR::L1::MemoryManager               &memoryManager,
                HiCR::L1::CommunicationManager        &communicationManager,
                std::shared_ptr<HiCR::L0::MemorySpace> bufferMemorySpace,
                const size_t                           channelCapacity,
                const size_t                           producerCount)
{
  // Getting required buffer sizes
  auto tokenBufferSize = HiCR::channel::fixedSize::Base::getTokenBufferSize(sizeof(ELEMENT_TYPE), channelCapacity);

  std::vector<HiCR::L1::CommunicationManager::globalKeyMemorySlotPair_t> tokenBuffers;
  std::vector<HiCR::L1::CommunicationManager::globalKeyMemorySlotPair_t> consumerCoordinationBuffers;
  std::vector<std::shared_ptr<HiCR::L0::GlobalMemorySlot>>               producerCoordinationBuffers;
  std::vector<std::shared_ptr<HiCR::L0::GlobalMemorySlot>>               globalTokenBuffers;
  std::vector<std::shared_ptr<HiCR::L0::LocalMemorySlot>>                localCoordinationBuffers;

  for (size_t i = 0; i < producerCount; i++)
  {
    // consumer needs to allocate #producers token buffers for #producers SPSCs
    auto tokenBufferSlot = memoryManager.allocateLocalMemorySlot(bufferMemorySpace, tokenBufferSize);
    tokenBuffers.push_back(std::make_pair(i, tokenBufferSlot));
    // consumer needs to allocate #producers consumer side coordination buffers for #producers SPSCs
    auto coordinationBufferSize = HiCR::channel::fixedSize::Base::getCoordinationBufferSize();
    auto coordinationBuffer     = memoryManager.allocateLocalMemorySlot(bufferMemorySpace, coordinationBufferSize);
    HiCR::channel::fixedSize::Base::initializeCoordinationBuffer(coordinationBuffer);
    localCoordinationBuffers.push_back(coordinationBuffer);
    consumerCoordinationBuffers.push_back(std::make_pair(i, coordinationBuffer));
  }

  // communicate to producers the token buffer references
  communicationManager.exchangeGlobalMemorySlots(TOKEN_TAG, tokenBuffers);
  communicationManager.fence(TOKEN_TAG);
  // get from producers their coordination buffer references
  communicationManager.exchangeGlobalMemorySlots(PRODUCER_COORDINATION_TAG, {});
  communicationManager.fence(PRODUCER_COORDINATION_TAG);
  // communicate to producers the consumer buffers
  communicationManager.exchangeGlobalMemorySlots(CONSUMER_COORDINATION_TAG, consumerCoordinationBuffers);
  communicationManager.fence(CONSUMER_COORDINATION_TAG);

  for (size_t i = 0; i < producerCount; i++)
  {
    auto globalTokenBufferSlot = communicationManager.getGlobalMemorySlot(TOKEN_TAG, i);
    globalTokenBuffers.push_back(globalTokenBufferSlot);
    auto producerCoordinationBuffer = communicationManager.getGlobalMemorySlot(PRODUCER_COORDINATION_TAG, i);
    producerCoordinationBuffers.push_back(producerCoordinationBuffer);
  }

  // Creating producer and consumer channels
  auto consumer = HiCR::channel::fixedSize::MPSC::nonlocking::Consumer(
    communicationManager, globalTokenBuffers, localCoordinationBuffers, producerCoordinationBuffers, sizeof(ELEMENT_TYPE), channelCapacity);

  // Calculating the expected message count
  size_t expectedMessageCount = MESSAGES_PER_PRODUCER * producerCount;
  size_t receivedMessageCount = 0;

  // Waiting for all messages to arrive, and printing them one by one
  while (receivedMessageCount < expectedMessageCount)
  {
    do {
      consumer.updateDepth();
    }
    while (consumer.isEmpty());

    // peek returns pair (channelID, position in channel)
    std::array<size_t, 2> peekResult = consumer.peek();
    auto                  channelId  = peekResult[0];
    auto                  pos        = peekResult[1];

    // Increasing received message counter
    receivedMessageCount++;

    auto          slot = tokenBuffers[channelId].second;
    ELEMENT_TYPE *ptr  = (ELEMENT_TYPE *)slot->getPointer();
    // Printing value
    printf("    [Consumer] Recv Value: %u  (%lu/%lu) Pos: %ld @ SPSC Channel %ld\n", ptr[pos], receivedMessageCount, expectedMessageCount, pos, channelId);

    // Disposing of printed value
    consumer.pop();
  }

  communicationManager.fence(TOKEN_TAG);
  communicationManager.fence(PRODUCER_COORDINATION_TAG);
  communicationManager.fence(CONSUMER_COORDINATION_TAG);

  for (size_t i = 0; i < producerCount; i++)
  {
    communicationManager.deregisterGlobalMemorySlot(globalTokenBuffers[i]);
    communicationManager.destroyGlobalMemorySlot(globalTokenBuffers[i]);
    memoryManager.freeLocalMemorySlot(globalTokenBuffers[i]->getSourceLocalMemorySlot());
    communicationManager.deregisterGlobalMemorySlot(producerCoordinationBuffers[i]);
    communicationManager.destroyGlobalMemorySlot(producerCoordinationBuffers[i]);
    memoryManager.freeLocalMemorySlot(localCoordinationBuffers[i]);
  }

  communicationManager.fence(TOKEN_TAG);
  communicationManager.fence(PRODUCER_COORDINATION_TAG);
  communicationManager.fence(CONSUMER_COORDINATION_TAG);
}
