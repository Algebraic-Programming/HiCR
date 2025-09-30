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
#include <hicr/frontends/channel/variableSize/mpsc/nonlocking/consumer.hpp>
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
  const size_t payloadSize       = sizeof(ELEMENT_TYPE);
  const size_t tokenSize         = sizeof(size_t);
  auto         sizesBufferSize   = HiCR::channel::variableSize::Base::getTokenBufferSize(tokenSize, channelCapacity);
  auto         payloadBufferSize = HiCR::channel::variableSize::Base::getTokenBufferSize(payloadSize, channelCapacity);
  // list of all consumer coordination buffers (= #producers); needed to construct consumer
  std::vector<std::shared_ptr<HiCR::GlobalMemorySlot>> coordinationBuffersForPayloadsAsSlots;
  std::vector<std::shared_ptr<HiCR::GlobalMemorySlot>> coordinationBuffersForCountsAsSlots;
  // list of all consumer coordination buffers (= #producers); needed to construct consumer
  std::vector<std::shared_ptr<HiCR::LocalMemorySlot>> internalCoordinationBuffersForCounts;
  std::vector<std::shared_ptr<HiCR::LocalMemorySlot>> internalCoordinationBuffersForPayloads;

  // Helper lists of coordination buffers and counts/payload slots
  // These are useful when constructing arguments for the exchange call
  std::vector<HiCR::CommunicationManager::globalKeyMemorySlotPair_t> consumerCoordinationBuffersForCounts;
  std::vector<HiCR::CommunicationManager::globalKeyMemorySlotPair_t> consumerCoordinationBuffersForPayloads;
  std::vector<HiCR::CommunicationManager::globalKeyMemorySlotPair_t> localBuffersForCounts;
  std::vector<HiCR::CommunicationManager::globalKeyMemorySlotPair_t> localBuffersForPayloads;
  // list of producer coordination buffers (= #producers); needed to construct consumer
  std::vector<std::shared_ptr<HiCR::GlobalMemorySlot>> producerCoordinationBuffersForCounts;
  std::vector<std::shared_ptr<HiCR::GlobalMemorySlot>> producerCoordinationBuffersForPayloads;
  // list of consumer buffers for counts (= #producers) and payloads (= #producers); needed to construct consumer
  std::vector<std::shared_ptr<HiCR::GlobalMemorySlot>> globalBuffersForPayloads;
  std::vector<std::shared_ptr<HiCR::GlobalMemorySlot>> globalBuffersForCounts;
  const size_t                                         coordinationBufferSize = HiCR::channel::variableSize::Base::getCoordinationBufferSize();

  // consumer needs to allocate #producers {size, payload} coord. buffers for #producers SPSCs
  for (size_t i = 0; i < producerCount; i++)
  {
    auto coordinationBufferForCounts   = coordinationMemoryManager.allocateLocalMemorySlot(coordinationMemorySpace, coordinationBufferSize);
    auto coordinationBufferForPayloads = coordinationMemoryManager.allocateLocalMemorySlot(coordinationMemorySpace, coordinationBufferSize);
    // very important to initialize coordination buffers, or undefined behaviour possible!
    HiCR::channel::variableSize::Base::initializeCoordinationBuffer(coordinationBufferForCounts);
    HiCR::channel::variableSize::Base::initializeCoordinationBuffer(coordinationBufferForPayloads);
    consumerCoordinationBuffersForCounts.push_back(std::make_pair(i, coordinationBufferForCounts));
    consumerCoordinationBuffersForPayloads.push_back(std::make_pair(i, coordinationBufferForPayloads));
    internalCoordinationBuffersForCounts.push_back(coordinationBufferForCounts);
    internalCoordinationBuffersForPayloads.push_back(coordinationBufferForPayloads);
    auto consumerSizesBuffer = coordinationMemoryManager.allocateLocalMemorySlot(coordinationMemorySpace, sizesBufferSize);
    localBuffersForCounts.push_back(std::make_pair(i, consumerSizesBuffer));
    auto consumerPayloadBuffer = payloadMemoryManager.allocateLocalMemorySlot(payloadMemorySpace, payloadBufferSize);
    localBuffersForPayloads.push_back(std::make_pair(i, consumerPayloadBuffer));
  }

  // communicate to producers the token buffer references
  coordinationCommunicationManager.exchangeGlobalMemorySlots(CONSUMER_COORDINATION_BUFFER_FOR_SIZES_KEY, consumerCoordinationBuffersForCounts);
  coordinationCommunicationManager.exchangeGlobalMemorySlots(CONSUMER_COORDINATION_BUFFER_FOR_PAYLOADS_KEY, consumerCoordinationBuffersForPayloads);
  coordinationCommunicationManager.exchangeGlobalMemorySlots(CONSUMER_TOKEN_KEY, localBuffersForCounts);
  payloadCommunicationManager.exchangeGlobalMemorySlots(CONSUMER_PAYLOAD_KEY, localBuffersForPayloads);

  coordinationCommunicationManager.fence(CONSUMER_COORDINATION_BUFFER_FOR_SIZES_KEY);
  coordinationCommunicationManager.fence(CONSUMER_COORDINATION_BUFFER_FOR_PAYLOADS_KEY);
  coordinationCommunicationManager.fence(CONSUMER_TOKEN_KEY);
  payloadCommunicationManager.fence(CONSUMER_PAYLOAD_KEY);

  // get from producers their coordination buffer references
  coordinationCommunicationManager.exchangeGlobalMemorySlots(PRODUCER_COORDINATION_BUFFER_FOR_SIZES_KEY, {});
  coordinationCommunicationManager.exchangeGlobalMemorySlots(PRODUCER_COORDINATION_BUFFER_FOR_PAYLOADS_KEY, {});

  coordinationCommunicationManager.fence(PRODUCER_COORDINATION_BUFFER_FOR_SIZES_KEY);
  coordinationCommunicationManager.fence(PRODUCER_COORDINATION_BUFFER_FOR_PAYLOADS_KEY);

  // get all global slot information required (local operations)
  for (size_t i = 0; i < producerCount; i++)
  {
    auto producerCoordinationBufferForCounts = coordinationCommunicationManager.getGlobalMemorySlot(PRODUCER_COORDINATION_BUFFER_FOR_SIZES_KEY, i);
    producerCoordinationBuffersForCounts.push_back(producerCoordinationBufferForCounts);
    auto producerCoordinationBufferForPayloads = coordinationCommunicationManager.getGlobalMemorySlot(PRODUCER_COORDINATION_BUFFER_FOR_PAYLOADS_KEY, i);
    producerCoordinationBuffersForPayloads.push_back(producerCoordinationBufferForPayloads);
    auto consumerCoordinationBufferForPayloads = coordinationCommunicationManager.getGlobalMemorySlot(CONSUMER_COORDINATION_BUFFER_FOR_PAYLOADS_KEY, i);
    coordinationBuffersForPayloadsAsSlots.push_back(consumerCoordinationBufferForPayloads);
    auto consumerCoordinationBufferForCounts = coordinationCommunicationManager.getGlobalMemorySlot(CONSUMER_COORDINATION_BUFFER_FOR_SIZES_KEY, i);
    coordinationBuffersForCountsAsSlots.push_back(consumerCoordinationBufferForCounts);
    auto consumerBufferForPayloads = payloadCommunicationManager.getGlobalMemorySlot(CONSUMER_PAYLOAD_KEY, i);
    globalBuffersForPayloads.push_back(consumerBufferForPayloads);
    auto consumerBufferForSizes = coordinationCommunicationManager.getGlobalMemorySlot(CONSUMER_TOKEN_KEY, i);
    globalBuffersForCounts.push_back(consumerBufferForSizes);
  }

  // Creating consumer channels
  auto consumer = new HiCR::channel::variableSize::MPSC::nonlocking::Consumer(coordinationCommunicationManager,
                                                                              payloadCommunicationManager,
                                                                              globalBuffersForPayloads,
                                                                              globalBuffersForCounts,
                                                                              internalCoordinationBuffersForCounts,
                                                                              internalCoordinationBuffersForPayloads,
                                                                              producerCoordinationBuffersForCounts,
                                                                              producerCoordinationBuffersForPayloads,
                                                                              /* E.g. if channel capacity is 10, and a payload datatype is 8 bytes,
                                                                        allocate a buffer of 80 bytes. This is just an estimate which may or may not
                                                                        be relevant in real applications
                                                                       */
                                                                              payloadSize * channelCapacity,
                                                                              payloadSize,
                                                                              channelCapacity);

  size_t poppedElems = 0;
  // Expect MESSAGES_PER_PRODUCER from each producer
  while (poppedElems < MESSAGES_PER_PRODUCER * producerCount)
  {
    // even if each SPSC's updateDepth is a NOP
    // it is essential to call updateDepth() on the MPSC
    // in order for it to update the depths of its SPSCs
    while (consumer->isEmpty()) { consumer->updateDepth(); }

    auto res       = consumer->peek();
    auto channelId = res[0];
    char prefix[64];
    sprintf(prefix, "CONSUMER @ channel %lu ", channelId);
    auto startIndex      = res[1];
    auto byteLen         = res[2];
    auto localMemorySlot = localBuffersForPayloads[channelId].second;

    Printer<ELEMENT_TYPE>::printBytes(prefix, localMemorySlot->getPointer(), channelCapacity * payloadSize, startIndex, byteLen);
    consumer->pop();
    poppedElems++;
  }

  // deregister global slots -- needs to be in synch at consumer and ALL producers
  for (size_t i = 0; i < producerCount; i++)
  {
    coordinationCommunicationManager.deregisterGlobalMemorySlot(globalBuffersForCounts[i]);
    coordinationCommunicationManager.deregisterGlobalMemorySlot(globalBuffersForPayloads[i]);
    coordinationCommunicationManager.deregisterGlobalMemorySlot(coordinationBuffersForCountsAsSlots[i]);
    coordinationCommunicationManager.deregisterGlobalMemorySlot(coordinationBuffersForPayloadsAsSlots[i]);
    coordinationCommunicationManager.deregisterGlobalMemorySlot(producerCoordinationBuffersForCounts[i]);
    coordinationCommunicationManager.deregisterGlobalMemorySlot(producerCoordinationBuffersForPayloads[i]);

    coordinationCommunicationManager.destroyGlobalMemorySlot(globalBuffersForCounts[i]);
    coordinationCommunicationManager.destroyGlobalMemorySlot(globalBuffersForPayloads[i]);
    coordinationCommunicationManager.destroyGlobalMemorySlot(coordinationBuffersForCountsAsSlots[i]);
    coordinationCommunicationManager.destroyGlobalMemorySlot(coordinationBuffersForPayloadsAsSlots[i]);
  }

  // Fences for global slots destructions/cleanup
  coordinationCommunicationManager.fence(CONSUMER_COORDINATION_BUFFER_FOR_SIZES_KEY);
  coordinationCommunicationManager.fence(CONSUMER_COORDINATION_BUFFER_FOR_PAYLOADS_KEY);
  coordinationCommunicationManager.fence(CONSUMER_TOKEN_KEY);
  coordinationCommunicationManager.fence(PRODUCER_COORDINATION_BUFFER_FOR_SIZES_KEY);
  coordinationCommunicationManager.fence(PRODUCER_COORDINATION_BUFFER_FOR_PAYLOADS_KEY);
  payloadCommunicationManager.fence(CONSUMER_PAYLOAD_KEY);

  for (size_t i = 0; i < producerCount; i++)
  {
    coordinationMemoryManager.freeLocalMemorySlot(internalCoordinationBuffersForCounts[i]);
    coordinationMemoryManager.freeLocalMemorySlot(internalCoordinationBuffersForPayloads[i]);
    coordinationMemoryManager.freeLocalMemorySlot(localBuffersForCounts[i].second);
    coordinationMemoryManager.freeLocalMemorySlot(localBuffersForPayloads[i].second);
  }
}
