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
#include <hicr/frontends/channel/variableSize/spsc/producer.hpp>

void producerFc(HiCR::MemoryManager               &coordinationMemoryManager,
                HiCR::MemoryManager               &payloadMemoryManager,
                HiCR::CommunicationManager        &coordinationCommunicationManager,
                HiCR::CommunicationManager        &payloadCommunicationManager,
                std::shared_ptr<HiCR::MemorySpace> coordinationMemorySpace,
                std::shared_ptr<HiCR::MemorySpace> payloadMemorySpace,
                const size_t                       channelCapacity)
{
  // Getting required buffer size
  auto coordinationBufferSize = HiCR::channel::variableSize::Base::getCoordinationBufferSize();

  // Allocating sizes buffer as a local memory slot
  auto coordinationBufferForCounts = coordinationMemoryManager.allocateLocalMemorySlot(coordinationMemorySpace, coordinationBufferSize);

  auto coordinationBufferForPayloads = coordinationMemoryManager.allocateLocalMemorySlot(coordinationMemorySpace, coordinationBufferSize);

  auto sizeInfoBuffer = coordinationMemoryManager.allocateLocalMemorySlot(coordinationMemorySpace, sizeof(size_t));

  // Initializing coordination buffers for message sizes and payloads (sets to zero the counters)
  HiCR::channel::variableSize::Base::initializeCoordinationBuffer(coordinationBufferForCounts);
  HiCR::channel::variableSize::Base::initializeCoordinationBuffer(coordinationBufferForPayloads);

  // Exchanging local memory slots to become global for them to be used by the remote end
  coordinationCommunicationManager.exchangeGlobalMemorySlots(CHANNEL_TAG,                                                                /* global tag */
                                                             {{PRODUCER_COORDINATION_BUFFER_FOR_SIZES_KEY, coordinationBufferForCounts}, /* key-slot pairs */
                                                              {PRODUCER_COORDINATION_BUFFER_FOR_PAYLOADS_KEY, coordinationBufferForPayloads}});

  payloadCommunicationManager.exchangeGlobalMemorySlots(CHANNEL_TAG, {});

  // Synchronizing so that all actors have finished registering their global memory slots
  coordinationCommunicationManager.fence(CHANNEL_TAG);
  payloadCommunicationManager.fence(CHANNEL_TAG);

  // Obtaining the globally exchanged memory slots
  auto sizesBuffer                           = coordinationCommunicationManager.getGlobalMemorySlot(CHANNEL_TAG, SIZES_BUFFER_KEY);
  auto producerCoordinationBufferForCounts   = coordinationCommunicationManager.getGlobalMemorySlot(CHANNEL_TAG, PRODUCER_COORDINATION_BUFFER_FOR_SIZES_KEY);
  auto producerCoordinationBufferForPayloads = coordinationCommunicationManager.getGlobalMemorySlot(CHANNEL_TAG, PRODUCER_COORDINATION_BUFFER_FOR_PAYLOADS_KEY);
  auto consumerCoordinationBufferForCounts   = coordinationCommunicationManager.getGlobalMemorySlot(CHANNEL_TAG, CONSUMER_COORDINATION_BUFFER_FOR_SIZES_KEY);
  auto consumerCoordinationBufferForPayloads = coordinationCommunicationManager.getGlobalMemorySlot(CHANNEL_TAG, CONSUMER_COORDINATION_BUFFER_FOR_PAYLOADS_KEY);
  auto payloadBuffer                         = payloadCommunicationManager.getGlobalMemorySlot(CHANNEL_TAG, CONSUMER_PAYLOAD_KEY);

  // Creating producer and consumer channels
  auto producer = HiCR::channel::variableSize::SPSC::Producer(coordinationCommunicationManager,
                                                              payloadCommunicationManager,
                                                              sizeInfoBuffer,
                                                              payloadBuffer,
                                                              sizesBuffer,
                                                              coordinationBufferForCounts,
                                                              coordinationBufferForPayloads,
                                                              consumerCoordinationBufferForCounts,
                                                              consumerCoordinationBufferForPayloads,
                                                              PAYLOAD_CAPACITY,
                                                              sizeof(ELEMENT_TYPE),
                                                              channelCapacity);

  // Allocating a send slot to put the values we want to communicate
  ELEMENT_TYPE sendBuffer[4]  = {0, 1, 2, 3};
  ELEMENT_TYPE sendBuffer2[3] = {4, 5, 6};
  ELEMENT_TYPE sendBuffer3[2] = {7, 8};
  auto         sendBufferPtr  = &sendBuffer;
  auto         sendBuffer2Ptr = &sendBuffer2;
  auto         sendBuffer3Ptr = &sendBuffer3;
  auto         sendSlot       = payloadMemoryManager.registerLocalMemorySlot(payloadMemorySpace, sendBufferPtr, sizeof(sendBuffer));
  auto         sendSlot2      = payloadMemoryManager.registerLocalMemorySlot(payloadMemorySpace, sendBuffer2Ptr, sizeof(sendBuffer2));
  auto         sendSlot3      = payloadMemoryManager.registerLocalMemorySlot(payloadMemorySpace, sendBuffer3Ptr, sizeof(sendBuffer3));

  // Pushing first batch
  producer.push(sendSlot);
  Printer<ELEMENT_TYPE>::printBytes("PRODUCER sent:", sendBufferPtr, sizeof(sendBuffer), 0, sizeof(sendBuffer));

  // If channel is full, wait until it frees up
  while (!producer.isEmpty()) producer.updateDepth();

  // Sending second batch
  producer.push(sendSlot2);
  Printer<ELEMENT_TYPE>::printBytes("PRODUCER sent:", sendBuffer2Ptr, sizeof(sendBuffer2), 0, sizeof(sendBuffer2));

  // If channel is full, wait until it frees up
  while (!producer.isEmpty()) producer.updateDepth();

  // Sending third batch (same as first batch)
  producer.push(sendSlot3);
  Printer<ELEMENT_TYPE>::printBytes("PRODUCER sent:", sendBuffer3Ptr, sizeof(sendBuffer3), 0, sizeof(sendBuffer3));

  // Synchronizing so that all actors have finished registering their global memory slots
  coordinationCommunicationManager.fence(CHANNEL_TAG);
  payloadCommunicationManager.fence(CHANNEL_TAG);

  // Destroying global slots (collective calls)
  coordinationCommunicationManager.destroyGlobalMemorySlot(sizesBuffer);
  coordinationCommunicationManager.destroyGlobalMemorySlot(producerCoordinationBufferForCounts);
  coordinationCommunicationManager.destroyGlobalMemorySlot(producerCoordinationBufferForPayloads);

  coordinationCommunicationManager.fence(CHANNEL_TAG);
  payloadCommunicationManager.fence(CHANNEL_TAG);

  // Freeing up local memory
  coordinationMemoryManager.freeLocalMemorySlot(coordinationBufferForCounts);
  coordinationMemoryManager.freeLocalMemorySlot(coordinationBufferForPayloads);
  coordinationMemoryManager.freeLocalMemorySlot(sizeInfoBuffer);
}
