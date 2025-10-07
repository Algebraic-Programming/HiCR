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
#include <hicr/core/globalMemorySlot.hpp>
#include <hicr/frontends/channel/fixedSize/mpsc/nonlocking/consumer.hpp>
#include "common.hpp"

void consumerFc(HiCR::MemoryManager               &coordinationMemoryManager,
                HiCR::MemoryManager               &payloadMemoryManager,
                HiCR::CommunicationManager        &coordinationCommunicationManager,
                HiCR::CommunicationManager        &payloadCommunicationManager,
                std::shared_ptr<HiCR::MemorySpace> coordinationMemorySpace,
                std::shared_ptr<HiCR::MemorySpace> payloadMemorySpace,
                const size_t                       channelCapacity,
                const size_t                       producerCount)
{
  // Getting required buffer sizes
  auto tokenBufferSize = HiCR::channel::fixedSize::Base::getTokenBufferSize(sizeof(ELEMENT_TYPE), channelCapacity);

  std::vector<HiCR::CommunicationManager::globalKeyMemorySlotPair_t> tokenBuffers;
  std::vector<HiCR::CommunicationManager::globalKeyMemorySlotPair_t> consumerCoordinationBuffers;
  std::vector<std::shared_ptr<HiCR::GlobalMemorySlot>>               producerCoordinationBuffers;
  std::vector<std::shared_ptr<HiCR::GlobalMemorySlot>>               globalTokenBuffers;
  std::vector<std::shared_ptr<HiCR::LocalMemorySlot>>                localCoordinationBuffers;

  for (size_t i = 0; i < producerCount; i++)
  {
    // consumer needs to allocate #producers token buffers for #producers SPSCs
    auto tokenBufferSlot = payloadMemoryManager.allocateLocalMemorySlot(payloadMemorySpace, tokenBufferSize);
    tokenBuffers.push_back(std::make_pair(i, tokenBufferSlot));
    // consumer needs to allocate #producers consumer side coordination buffers for #producers SPSCs
    auto coordinationBufferSize = HiCR::channel::fixedSize::Base::getCoordinationBufferSize();
    auto coordinationBuffer     = coordinationMemoryManager.allocateLocalMemorySlot(coordinationMemorySpace, coordinationBufferSize);
    HiCR::channel::fixedSize::Base::initializeCoordinationBuffer(coordinationBuffer);
    localCoordinationBuffers.push_back(coordinationBuffer);
    consumerCoordinationBuffers.push_back(std::make_pair(i, coordinationBuffer));
  }

  // communicate to producers the token buffer references
  payloadCommunicationManager.exchangeGlobalMemorySlots(TOKEN_TAG, tokenBuffers);
  payloadCommunicationManager.fence(TOKEN_TAG);
  // get from producers their coordination buffer references
  coordinationCommunicationManager.exchangeGlobalMemorySlots(PRODUCER_COORDINATION_TAG, {});
  coordinationCommunicationManager.fence(PRODUCER_COORDINATION_TAG);
  // communicate to producers the consumer buffers
  coordinationCommunicationManager.exchangeGlobalMemorySlots(CONSUMER_COORDINATION_TAG, consumerCoordinationBuffers);
  coordinationCommunicationManager.fence(CONSUMER_COORDINATION_TAG);

  for (size_t i = 0; i < producerCount; i++)
  {
    auto globalTokenBufferSlot = payloadCommunicationManager.getGlobalMemorySlot(TOKEN_TAG, i);
    globalTokenBuffers.push_back(globalTokenBufferSlot);
    auto producerCoordinationBuffer = coordinationCommunicationManager.getGlobalMemorySlot(PRODUCER_COORDINATION_TAG, i);
    producerCoordinationBuffers.push_back(producerCoordinationBuffer);
  }

  // Creating producer and consumer channels
  auto consumer = HiCR::channel::fixedSize::MPSC::nonlocking::Consumer(
    coordinationCommunicationManager, payloadCommunicationManager, globalTokenBuffers, localCoordinationBuffers, producerCoordinationBuffers, sizeof(ELEMENT_TYPE), channelCapacity);

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

  payloadCommunicationManager.fence(TOKEN_TAG);
  coordinationCommunicationManager.fence(PRODUCER_COORDINATION_TAG);
  coordinationCommunicationManager.fence(CONSUMER_COORDINATION_TAG);

  for (size_t i = 0; i < producerCount; i++)
  {
    payloadCommunicationManager.deregisterGlobalMemorySlot(globalTokenBuffers[i]);
    payloadMemoryManager.freeLocalMemorySlot(globalTokenBuffers[i]->getSourceLocalMemorySlot());
    coordinationCommunicationManager.destroyGlobalMemorySlot(globalTokenBuffers[i]);
    coordinationCommunicationManager.deregisterGlobalMemorySlot(producerCoordinationBuffers[i]);
    coordinationCommunicationManager.destroyGlobalMemorySlot(producerCoordinationBuffers[i]);
    coordinationMemoryManager.freeLocalMemorySlot(localCoordinationBuffers[i]);
  }

  payloadCommunicationManager.fence(TOKEN_TAG);
  coordinationCommunicationManager.fence(PRODUCER_COORDINATION_TAG);
  coordinationCommunicationManager.fence(CONSUMER_COORDINATION_TAG);
}
