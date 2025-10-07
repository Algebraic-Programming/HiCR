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

#include "common.hpp"
#include <hicr/core/memoryManager.hpp>
#include <hicr/core/communicationManager.hpp>
#include <hicr/frontends/channel/fixedSize/spsc/producer.hpp>

void producerFc(HiCR::MemoryManager               &coordinationMemoryManager,
                HiCR::MemoryManager               &payloadMemoryManager,
                HiCR::CommunicationManager        &coordinationCommunicationManager,
                HiCR::CommunicationManager        &payloadCommunicationManager,
                std::shared_ptr<HiCR::MemorySpace> coordinationMemorySpace,
                std::shared_ptr<HiCR::MemorySpace> payloadMemorySpace,
                const size_t                       channelCapacity)
{
  // Getting required buffer size
  auto coordinationBufferSize = HiCR::channel::fixedSize::Base::getCoordinationBufferSize();

  // Allocating token buffer as a local memory slot
  auto coordinationBuffer = coordinationMemoryManager.allocateLocalMemorySlot(coordinationMemorySpace, coordinationBufferSize);

  // Initializing coordination buffer (sets to zero the counters)
  HiCR::channel::fixedSize::Base::initializeCoordinationBuffer(coordinationBuffer);

  // Exchanging local memory slots to become global for them to be used by the remote end
  coordinationCommunicationManager.exchangeGlobalMemorySlots(CHANNEL_TAG, {{PRODUCER_COORDINATION_BUFFER_KEY, coordinationBuffer}});
  payloadCommunicationManager.exchangeGlobalMemorySlots(CHANNEL_TAG, {});

  // Synchronizing so that all actors have finished registering their global memory slots
  coordinationCommunicationManager.fence(CHANNEL_TAG);
  payloadCommunicationManager.fence(CHANNEL_TAG);

  // Obtaining the globally exchanged memory slots
  auto tokenBuffer                = payloadCommunicationManager.getGlobalMemorySlot(CHANNEL_TAG, TOKEN_BUFFER_KEY);
  auto producerCoordinationBuffer = coordinationCommunicationManager.getGlobalMemorySlot(CHANNEL_TAG, PRODUCER_COORDINATION_BUFFER_KEY);
  auto consumerCoordinationBuffer = coordinationCommunicationManager.getGlobalMemorySlot(CHANNEL_TAG, CONSUMER_COORDINATION_BUFFER_KEY);

  // Creating producer and consumer channels
  auto producer = HiCR::channel::fixedSize::SPSC::Producer(
    coordinationCommunicationManager, payloadCommunicationManager, tokenBuffer, coordinationBuffer, consumerCoordinationBuffer, sizeof(ELEMENT_TYPE), channelCapacity);

  // Allocating a send slot to put the values we want to communicate
  ELEMENT_TYPE sendBuffer    = 0;
  auto         sendBufferPtr = &sendBuffer;
  auto         sendSlot      = payloadMemoryManager.registerLocalMemorySlot(payloadMemorySpace, sendBufferPtr, sizeof(ELEMENT_TYPE));

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
  coordinationCommunicationManager.fence(CHANNEL_TAG);
  payloadCommunicationManager.fence(CHANNEL_TAG);

  // De-registering global slots
  payloadCommunicationManager.deregisterGlobalMemorySlot(tokenBuffer);
  coordinationCommunicationManager.deregisterGlobalMemorySlot(producerCoordinationBuffer);
  coordinationCommunicationManager.deregisterGlobalMemorySlot(consumerCoordinationBuffer);

  // Destroying global slots
  coordinationCommunicationManager.destroyGlobalMemorySlot(consumerCoordinationBuffer);

  // Fence for the destroys to occur
  coordinationCommunicationManager.fence(CHANNEL_TAG);
  payloadCommunicationManager.fence(CHANNEL_TAG);

  // Freeing up local memory
  coordinationMemoryManager.freeLocalMemorySlot(coordinationBuffer);
}
