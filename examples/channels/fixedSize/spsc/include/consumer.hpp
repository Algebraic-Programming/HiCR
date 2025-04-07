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
#include <hicr/frontends/channel/fixedSize/spsc/consumer.hpp>

void consumerFc(HiCR::MemoryManager               &memoryManager,
                HiCR::CommunicationManager        &communicationManager,
                std::shared_ptr<HiCR::MemorySpace> bufferMemorySpace,
                const size_t                           channelCapacity)
{
  // Getting required buffer sizes
  auto tokenBufferSize = HiCR::channel::fixedSize::Base::getTokenBufferSize(sizeof(ELEMENT_TYPE), channelCapacity);

  // Allocating token buffer as a local memory slot
  auto tokenBufferSlot = memoryManager.allocateLocalMemorySlot(bufferMemorySpace, tokenBufferSize);

  // Getting required buffer size
  auto coordinationBufferSize = HiCR::channel::fixedSize::Base::getCoordinationBufferSize();

  // Allocating coordination buffer as a local memory slot
  auto coordinationBuffer = memoryManager.allocateLocalMemorySlot(bufferMemorySpace, coordinationBufferSize);

  // Initializing coordination buffer (sets to zero the counters)
  HiCR::channel::fixedSize::Base::initializeCoordinationBuffer(coordinationBuffer);

  // Exchanging local memory slots to become global for them to be used by the remote end
  communicationManager.exchangeGlobalMemorySlots(CHANNEL_TAG, {{TOKEN_BUFFER_KEY, tokenBufferSlot}, {CONSUMER_COORDINATION_BUFFER_KEY, coordinationBuffer}});

  // Synchronizing so that all actors have finished registering their global memory slots
  communicationManager.fence(CHANNEL_TAG);

  // Obtaining the globally exchanged memory slots
  auto globalTokenBufferSlot      = communicationManager.getGlobalMemorySlot(CHANNEL_TAG, TOKEN_BUFFER_KEY);
  auto producerCoordinationBuffer = communicationManager.getGlobalMemorySlot(CHANNEL_TAG, PRODUCER_COORDINATION_BUFFER_KEY);
  auto consumerCoordinationBuffer = communicationManager.getGlobalMemorySlot(CHANNEL_TAG, CONSUMER_COORDINATION_BUFFER_KEY);

  // Creating producer and consumer channels
  auto consumer =
    HiCR::channel::fixedSize::SPSC::Consumer(communicationManager, globalTokenBufferSlot, coordinationBuffer, producerCoordinationBuffer, sizeof(ELEMENT_TYPE), channelCapacity);

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
  communicationManager.fence(CHANNEL_TAG);

  // De-registering global slots
  communicationManager.deregisterGlobalMemorySlot(globalTokenBufferSlot);
  communicationManager.deregisterGlobalMemorySlot(producerCoordinationBuffer);
  communicationManager.deregisterGlobalMemorySlot(consumerCoordinationBuffer);

  // Destroying global slots
  communicationManager.destroyGlobalMemorySlot(globalTokenBufferSlot);
  communicationManager.destroyGlobalMemorySlot(producerCoordinationBuffer);
  communicationManager.destroyGlobalMemorySlot(consumerCoordinationBuffer);

  // Fence for the destroys to occur
  communicationManager.fence(CHANNEL_TAG);

  // Freeing up local memory
  memoryManager.freeLocalMemorySlot(tokenBufferSlot);
  memoryManager.freeLocalMemorySlot(coordinationBuffer);
}
