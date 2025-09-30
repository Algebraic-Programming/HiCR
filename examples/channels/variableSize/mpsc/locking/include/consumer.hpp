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

#include <hicr/core/memoryManager.hpp>
#include <hicr/core/communicationManager.hpp>
#include <hicr/frontends/channel/variableSize/mpsc/locking/consumer.hpp>
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
  auto sizesBufferSize = HiCR::channel::variableSize::Base::getTokenBufferSize(sizeof(size_t), channelCapacity);

  // Allocating sizes buffer as a local memory slot
  auto sizesBufferSlot = coordinationMemoryManager.allocateLocalMemorySlot(coordinationMemorySpace, sizesBufferSize);

  const size_t payloadCapacity = channelCapacity * sizeof(ELEMENT_TYPE);

  // Allocating payload buffer as a local memory slot
  auto payloadBufferSlot = payloadMemoryManager.allocateLocalMemorySlot(payloadMemorySpace, payloadCapacity);

  // Getting required buffer size
  auto coordinationBufferSize = HiCR::channel::variableSize::Base::getCoordinationBufferSize();

  // Allocating coordination buffer for internal message size metadata
  auto coordinationBufferForCounts = coordinationMemoryManager.allocateLocalMemorySlot(coordinationMemorySpace, coordinationBufferSize);

  // Allocating coordination buffer for internal payload metadata
  auto coordinationBufferForPayloads = coordinationMemoryManager.allocateLocalMemorySlot(coordinationMemorySpace, coordinationBufferSize);

  // Initializing coordination buffer (sets to zero the counters)
  HiCR::channel::variableSize::Base::initializeCoordinationBuffer(coordinationBufferForCounts);

  HiCR::channel::variableSize::Base::initializeCoordinationBuffer(coordinationBufferForPayloads);

  // Exchanging local memory slots to become global for them to be used by the remote end
  coordinationCommunicationManager.exchangeGlobalMemorySlots(CHANNEL_TAG,
                                                             {{SIZES_BUFFER_KEY, sizesBufferSlot},
                                                              {CONSUMER_COORDINATION_BUFFER_FOR_SIZES_KEY, coordinationBufferForCounts},
                                                              {CONSUMER_COORDINATION_BUFFER_FOR_PAYLOADS_KEY, coordinationBufferForPayloads}});
  payloadCommunicationManager.exchangeGlobalMemorySlots(CHANNEL_TAG, {{CONSUMER_PAYLOAD_KEY, payloadBufferSlot}});

  // Synchronizing so that all actors have finished registering their global memory slots
  coordinationCommunicationManager.fence(CHANNEL_TAG);
  payloadCommunicationManager.fence(CHANNEL_TAG);

  // Obtaining the globally exchanged memory slots
  auto globalSizesBufferSlot                 = coordinationCommunicationManager.getGlobalMemorySlot(CHANNEL_TAG, SIZES_BUFFER_KEY);
  auto consumerCoordinationBufferForCounts   = coordinationCommunicationManager.getGlobalMemorySlot(CHANNEL_TAG, CONSUMER_COORDINATION_BUFFER_FOR_SIZES_KEY);
  auto consumerCoordinationBufferForPayloads = coordinationCommunicationManager.getGlobalMemorySlot(CHANNEL_TAG, CONSUMER_COORDINATION_BUFFER_FOR_PAYLOADS_KEY);
  auto globalPayloadBuffer                   = payloadCommunicationManager.getGlobalMemorySlot(CHANNEL_TAG, CONSUMER_PAYLOAD_KEY);

  // Creating producer and consumer channels
  auto consumer = HiCR::channel::variableSize::MPSC::locking::Consumer(coordinationCommunicationManager,
                                                                       payloadCommunicationManager,
                                                                       globalPayloadBuffer,   /* payloadBuffer */
                                                                       globalSizesBufferSlot, /* tokenSizeBuffer */
                                                                       coordinationBufferForCounts,
                                                                       coordinationBufferForPayloads,
                                                                       consumerCoordinationBufferForCounts,
                                                                       consumerCoordinationBufferForPayloads,
                                                                       payloadCapacity,
                                                                       channelCapacity);

  // Getting internal pointer of the token buffer slot
  auto payloadBufferPtr = (ELEMENT_TYPE *)payloadBufferSlot->getPointer();

  // Calculating the expected message count
  size_t expectedMessageCount = MESSAGES_PER_PRODUCER * producerCount;

  size_t poppedElems = 0;
  char   prefix[64]  = {'\0'};
  while (poppedElems < expectedMessageCount)
  {
    while (consumer.isEmpty()); // spin
    auto res = consumer.peek();
    /**
     * An example-specific way to deduce the sender rank
     */
    sprintf(prefix, "CONSUMER receives from PRODUCER %u:", payloadBufferPtr[res[0] / sizeof(ELEMENT_TYPE)]);

    Printer<ELEMENT_TYPE>::printBytes(prefix, payloadBufferPtr, payloadCapacity, res[0], res[1]);

    while (consumer.pop() == false);

    poppedElems++;
  }

  // Synchronizing so that all actors have finished registering their global memory slots
  coordinationCommunicationManager.fence(CHANNEL_TAG);
  payloadCommunicationManager.fence(CHANNEL_TAG);

  // De-registering global slots
  coordinationCommunicationManager.deregisterGlobalMemorySlot(globalSizesBufferSlot);
  coordinationCommunicationManager.deregisterGlobalMemorySlot(globalPayloadBuffer);
  coordinationCommunicationManager.deregisterGlobalMemorySlot(consumerCoordinationBufferForCounts);
  coordinationCommunicationManager.deregisterGlobalMemorySlot(consumerCoordinationBufferForPayloads);

  // Destroying global slots (collective calls)
  coordinationCommunicationManager.destroyGlobalMemorySlot(globalSizesBufferSlot);
  coordinationCommunicationManager.destroyGlobalMemorySlot(consumerCoordinationBufferForCounts);
  coordinationCommunicationManager.destroyGlobalMemorySlot(consumerCoordinationBufferForPayloads);
  payloadCommunicationManager.destroyGlobalMemorySlot(globalPayloadBuffer);

  // Fence for Global Slot destruction/cleanup
  coordinationCommunicationManager.fence(CHANNEL_TAG);
  payloadCommunicationManager.fence(CHANNEL_TAG);

  // Freeing up local memory
  payloadMemoryManager.freeLocalMemorySlot(payloadBufferSlot);
  coordinationMemoryManager.freeLocalMemorySlot(sizesBufferSlot);
  coordinationMemoryManager.freeLocalMemorySlot(coordinationBufferForCounts);
  coordinationMemoryManager.freeLocalMemorySlot(coordinationBufferForPayloads);
}
