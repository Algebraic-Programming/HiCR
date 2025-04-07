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
#include <hicr/frontends/channel/variableSize/mpsc/nonlocking/producer.hpp>
#include "common.hpp"

void producerFc(HiCR::MemoryManager               &memoryManager,
                HiCR::CommunicationManager        &communicationManager,
                std::shared_ptr<HiCR::MemorySpace> bufferMemorySpace,
                const size_t                           channelCapacity,
                const size_t                           producerId,
                const size_t                           producerCount)
{
  const size_t payloadCapacity = channelCapacity * sizeof(ELEMENT_TYPE);

  // local slots at producer side for coordination buffers; needed to construct producer
  auto coordinationBufferSize        = HiCR::channel::variableSize::Base::getCoordinationBufferSize();
  auto coordinationBufferForCounts   = memoryManager.allocateLocalMemorySlot(bufferMemorySpace, coordinationBufferSize);
  auto coordinationBufferForPayloads = memoryManager.allocateLocalMemorySlot(bufferMemorySpace, coordinationBufferSize);
  HiCR::channel::variableSize::Base::initializeCoordinationBuffer(coordinationBufferForCounts);
  HiCR::channel::variableSize::Base::initializeCoordinationBuffer(coordinationBufferForPayloads);

  // list of all consumer coordination buffers; only the one associated with the producerId
  // is needed to construct producer; the rest are needed for synchronous deregister of global slots
  std::vector<std::shared_ptr<HiCR::GlobalMemorySlot>> coordinationBuffersForPayloadsAsSlots;
  std::vector<std::shared_ptr<HiCR::GlobalMemorySlot>> coordinationBuffersForCountsAsSlots;

  // list of all producer coordination buffers as global slots; needed for synchronous deregister of global slots
  std::vector<std::shared_ptr<HiCR::GlobalMemorySlot>> producerCoordinationBuffersForCounts;
  std::vector<std::shared_ptr<HiCR::GlobalMemorySlot>> producerCoordinationBuffersForPayloads;

  // list of all consumer buffers for counts and payloads;  only the one associated with the producerId
  // is needed to construct producer; the rest are needed for synchronous deregister of global slots
  std::vector<std::shared_ptr<HiCR::GlobalMemorySlot>> globalBuffersForPayloads;
  std::vector<std::shared_ptr<HiCR::GlobalMemorySlot>> globalBuffersForCounts;
  auto                                                     countsBuffer = memoryManager.allocateLocalMemorySlot(bufferMemorySpace, sizeof(size_t));

  // get from consumer global count/payload slots
  communicationManager.exchangeGlobalMemorySlots(CONSUMER_COORDINATION_BUFFER_FOR_SIZES_KEY, {});
  communicationManager.exchangeGlobalMemorySlots(CONSUMER_COORDINATION_BUFFER_FOR_PAYLOADS_KEY, {});
  communicationManager.exchangeGlobalMemorySlots(CONSUMER_TOKEN_KEY, {});
  communicationManager.exchangeGlobalMemorySlots(CONSUMER_PAYLOAD_KEY, {});

  communicationManager.fence(CONSUMER_COORDINATION_BUFFER_FOR_SIZES_KEY);
  communicationManager.fence(CONSUMER_COORDINATION_BUFFER_FOR_PAYLOADS_KEY);
  communicationManager.fence(CONSUMER_TOKEN_KEY);
  communicationManager.fence(CONSUMER_PAYLOAD_KEY);

  // send to consumers my coordination buffers
  communicationManager.exchangeGlobalMemorySlots(PRODUCER_COORDINATION_BUFFER_FOR_SIZES_KEY, {{producerId, coordinationBufferForCounts}});
  communicationManager.exchangeGlobalMemorySlots(PRODUCER_COORDINATION_BUFFER_FOR_PAYLOADS_KEY, {{producerId, coordinationBufferForPayloads}});
  communicationManager.fence(PRODUCER_COORDINATION_BUFFER_FOR_SIZES_KEY);
  communicationManager.fence(PRODUCER_COORDINATION_BUFFER_FOR_PAYLOADS_KEY);

  // get all global slot information required (local operations)
  for (size_t i = 0; i < producerCount; i++)
  {
    auto producerCoordinationBufferForCounts = communicationManager.getGlobalMemorySlot(PRODUCER_COORDINATION_BUFFER_FOR_SIZES_KEY, i);
    producerCoordinationBuffersForCounts.push_back(producerCoordinationBufferForCounts);
    auto producerCoordinationBufferForPayloads = communicationManager.getGlobalMemorySlot(PRODUCER_COORDINATION_BUFFER_FOR_PAYLOADS_KEY, i);
    producerCoordinationBuffersForPayloads.push_back(producerCoordinationBufferForPayloads);
    auto consumerCoordinationBufferForPayloads = communicationManager.getGlobalMemorySlot(CONSUMER_COORDINATION_BUFFER_FOR_PAYLOADS_KEY, i);
    coordinationBuffersForPayloadsAsSlots.push_back(consumerCoordinationBufferForPayloads);
    auto consumerCoordinationBufferForCounts = communicationManager.getGlobalMemorySlot(CONSUMER_COORDINATION_BUFFER_FOR_SIZES_KEY, i);
    coordinationBuffersForCountsAsSlots.push_back(consumerCoordinationBufferForCounts);
    auto consumerBufferForPayloads = communicationManager.getGlobalMemorySlot(CONSUMER_PAYLOAD_KEY, i);
    globalBuffersForPayloads.push_back(consumerBufferForPayloads);
    auto consumerBufferForSizes = communicationManager.getGlobalMemorySlot(CONSUMER_TOKEN_KEY, i);
    globalBuffersForCounts.push_back(consumerBufferForSizes);
  }

  // Creating producer channel
  auto producer = HiCR::channel::variableSize::MPSC::nonlocking::Producer(communicationManager,
                                                                          countsBuffer,
                                                                          globalBuffersForPayloads[producerId],
                                                                          globalBuffersForCounts[producerId],
                                                                          coordinationBufferForCounts,
                                                                          coordinationBufferForPayloads,
                                                                          coordinationBuffersForCountsAsSlots[producerId],
                                                                          coordinationBuffersForPayloadsAsSlots[producerId],
                                                                          payloadCapacity,
                                                                          sizeof(ELEMENT_TYPE),
                                                                          channelCapacity);

  auto producerIdAsUnsigned = static_cast<ELEMENT_TYPE>(producerId);

  // Allocating 3 different-sized slots to push to consumer
  ELEMENT_TYPE                                   sendBuffer1[5] = {42, 43, 44, 45, 46};
  ELEMENT_TYPE                                   sendBuffer2[4] = {42, 43, 44, 45};
  ELEMENT_TYPE                                   sendBuffer3[3] = {42, 43, 44};
  ELEMENT_TYPE                                   sendBuffer4[2] = {42, 43};
  ELEMENT_TYPE                                   sendBuffer5[1] = {42};
  std::vector<std::pair<ELEMENT_TYPE *, size_t>> elements;
  elements.push_back(std::make_pair(sendBuffer1, 5));
  elements.push_back(std::make_pair(sendBuffer2, 4));
  elements.push_back(std::make_pair(sendBuffer3, 3));
  elements.push_back(std::make_pair(sendBuffer4, 2));
  elements.push_back(std::make_pair(sendBuffer5, 1));

  char prefix[64] = {'\0'};
  sprintf(prefix, "PRODUCER %u sent:", producerIdAsUnsigned);

  for (size_t i = 0; i < MESSAGES_PER_PRODUCER; i++)
  {
    size_t nextElemSize = elements[i].second * sizeof(ELEMENT_TYPE);
    auto   sendSlot     = memoryManager.registerLocalMemorySlot(bufferMemorySpace, elements[i].first, nextElemSize);

    // is the message sizes buffer full?
    // Note: it might be necessary sometimes to also check the payload buffers
    // as they might overflow independently of the message size buffers
    while (producer.isFull()) { producer.updateDepth(); }

    producer.push(sendSlot);
    Printer<ELEMENT_TYPE>::printBytes(prefix, elements[i].first, payloadCapacity, 0, nextElemSize);
  }

  // deregister global slots -- needs to be in synch at consumer and ALL producers
  for (size_t i = 0; i < producerCount; i++)
  {
    communicationManager.deregisterGlobalMemorySlot(globalBuffersForCounts[i]);
    communicationManager.deregisterGlobalMemorySlot(globalBuffersForPayloads[i]);
    communicationManager.deregisterGlobalMemorySlot(coordinationBuffersForCountsAsSlots[i]);
    communicationManager.deregisterGlobalMemorySlot(coordinationBuffersForPayloadsAsSlots[i]);
    communicationManager.deregisterGlobalMemorySlot(producerCoordinationBuffersForCounts[i]);
    communicationManager.deregisterGlobalMemorySlot(producerCoordinationBuffersForPayloads[i]);
  }

  communicationManager.destroyGlobalMemorySlot(producerCoordinationBuffersForCounts[producerId]);
  communicationManager.destroyGlobalMemorySlot(producerCoordinationBuffersForPayloads[producerId]);

  // Fences for global slots destructions/cleanup
  communicationManager.fence(CONSUMER_COORDINATION_BUFFER_FOR_SIZES_KEY);
  communicationManager.fence(CONSUMER_COORDINATION_BUFFER_FOR_PAYLOADS_KEY);
  communicationManager.fence(CONSUMER_TOKEN_KEY);
  communicationManager.fence(CONSUMER_PAYLOAD_KEY);
  communicationManager.fence(PRODUCER_COORDINATION_BUFFER_FOR_SIZES_KEY);
  communicationManager.fence(PRODUCER_COORDINATION_BUFFER_FOR_PAYLOADS_KEY);

  memoryManager.freeLocalMemorySlot(coordinationBufferForCounts);
  memoryManager.freeLocalMemorySlot(coordinationBufferForPayloads);
  memoryManager.freeLocalMemorySlot(countsBuffer);
}
