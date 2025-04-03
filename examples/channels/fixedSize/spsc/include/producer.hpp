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
#include <hicr/core/L1/memoryManager.hpp>
#include <hicr/core/L1/communicationManager.hpp>
#include <hicr/frontends/channel/fixedSize/spsc/producer.hpp>

void producerFc(HiCR::L1::MemoryManager               &memoryManager,
                HiCR::L1::CommunicationManager        &communicationManager,
                std::shared_ptr<HiCR::L0::MemorySpace> bufferMemorySpace,
                const size_t                           channelCapacity)
{
  // Getting required buffer size
  auto coordinationBufferSize = HiCR::channel::fixedSize::Base::getCoordinationBufferSize();

  // Allocating token buffer as a local memory slot
  auto coordinationBuffer = memoryManager.allocateLocalMemorySlot(bufferMemorySpace, coordinationBufferSize);

  // Initializing coordination buffer (sets to zero the counters)
  HiCR::channel::fixedSize::Base::initializeCoordinationBuffer(coordinationBuffer);

  // Exchanging local memory slots to become global for them to be used by the remote end
  communicationManager.exchangeGlobalMemorySlots(CHANNEL_TAG, {{PRODUCER_COORDINATION_BUFFER_KEY, coordinationBuffer}});

  // Synchronizing so that all actors have finished registering their global memory slots
  communicationManager.fence(CHANNEL_TAG);

  // Obtaining the globally exchanged memory slots
  auto tokenBuffer                = communicationManager.getGlobalMemorySlot(CHANNEL_TAG, TOKEN_BUFFER_KEY);
  auto producerCoordinationBuffer = communicationManager.getGlobalMemorySlot(CHANNEL_TAG, PRODUCER_COORDINATION_BUFFER_KEY);
  auto consumerCoordinationBuffer = communicationManager.getGlobalMemorySlot(CHANNEL_TAG, CONSUMER_COORDINATION_BUFFER_KEY);

  // Creating producer and consumer channels
  auto producer =
    HiCR::channel::fixedSize::SPSC::Producer(communicationManager, tokenBuffer, coordinationBuffer, consumerCoordinationBuffer, sizeof(ELEMENT_TYPE), channelCapacity);

  // Allocating a send slot to put the values we want to communicate
  ELEMENT_TYPE sendBuffer    = 0;
  auto         sendBufferPtr = &sendBuffer;
  auto         sendSlot      = memoryManager.registerLocalMemorySlot(bufferMemorySpace, sendBufferPtr, sizeof(ELEMENT_TYPE));

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
  communicationManager.fence(CHANNEL_TAG);

  // De-registering global slots
  communicationManager.deregisterGlobalMemorySlot(tokenBuffer);
  communicationManager.deregisterGlobalMemorySlot(producerCoordinationBuffer);
  communicationManager.deregisterGlobalMemorySlot(consumerCoordinationBuffer);

  // Destroying global slots
  communicationManager.destroyGlobalMemorySlot(consumerCoordinationBuffer);

  // Fence for the destroys to occur
  communicationManager.fence(CHANNEL_TAG);

  // Freeing up local memory
  memoryManager.freeLocalMemorySlot(coordinationBuffer);
}
