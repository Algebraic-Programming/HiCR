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
#include <hicr/frontends/channel/fixedSize/mpsc/locking/producer.hpp>
#include "common.hpp"

void producerFc(HiCR::MemoryManager               &memoryManager,
                HiCR::CommunicationManager        &communicationManager,
                std::shared_ptr<HiCR::MemorySpace> bufferMemorySpace,
                const size_t                       channelCapacity,
                const size_t                       producerId)
{
  // Getting required buffer size
  auto coordinationBufferSize = HiCR::channel::fixedSize::Base::getCoordinationBufferSize();

  // Registering token buffer as a local memory slot
  auto coordinationBuffer = memoryManager.allocateLocalMemorySlot(bufferMemorySpace, coordinationBufferSize);

  // Initializing coordination buffer (sets to zero the counters)
  HiCR::channel::fixedSize::Base::initializeCoordinationBuffer(coordinationBuffer);

  // Exchanging local memory slots to become global for them to be used by the remote end
  communicationManager.exchangeGlobalMemorySlots(CHANNEL_TAG, {});

  // Synchronizing so that all actors have finished registering their global memory slots
  communicationManager.fence(CHANNEL_TAG);

  // Obtaining the globally exchanged memory slots
  auto globalTokenBufferSlot      = communicationManager.getGlobalMemorySlot(CHANNEL_TAG, TOKEN_BUFFER_KEY);
  auto consumerCoordinationBuffer = communicationManager.getGlobalMemorySlot(CHANNEL_TAG, CONSUMER_COORDINATION_BUFFER_KEY);

  // Creating producer and consumer channels
  auto producer = HiCR::channel::fixedSize::MPSC::locking::Producer(communicationManager,
                                                                    globalTokenBufferSlot, /* tokenBuffer */
                                                                    coordinationBuffer,    /* internalCoordinationBuffer */
                                                                    consumerCoordinationBuffer,
                                                                    sizeof(ELEMENT_TYPE),
                                                                    channelCapacity);

  // Allocating a send slot to put the values we want to communicate
  ELEMENT_TYPE sendBuffer    = 0;
  auto         sendBufferPtr = &sendBuffer;
  auto         sendSlot      = memoryManager.registerLocalMemorySlot(bufferMemorySpace, sendBufferPtr, sizeof(ELEMENT_TYPE));

  // Pushing values to the channel, one by one, suspending when/if the channel is full
  for (size_t i = 0; i < MESSAGES_PER_PRODUCER; i++)
  {
    // Calculating message value
    sendBuffer = 42 + i;

    // Trying to push (if the consumer buffer is busy or full, it will fail and try again)
    while (producer.push(sendSlot) == false)
      ;

    // Printing value sent
    printf("[Producer %03lu] Sent Value: %u\n", producerId, *sendBufferPtr);
  }

  // Synchronizing so that all actors have finished registering their global memory slots
  communicationManager.fence(CHANNEL_TAG);

  // De-registering global slots
  communicationManager.deregisterGlobalMemorySlot(globalTokenBufferSlot);
  communicationManager.deregisterGlobalMemorySlot(consumerCoordinationBuffer);

  // Destroying global slots
  communicationManager.fence(CHANNEL_TAG);

  // Freeing up local memory
  memoryManager.freeLocalMemorySlot(coordinationBuffer);
}
