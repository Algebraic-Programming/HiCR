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
#include <hicr/frontends/channel/variableSize/mpsc/locking/producer.hpp>
#include "common.hpp"

void producerFc(HiCR::MemoryManager               &coordinationMemoryManager,
                HiCR::MemoryManager               &payloadMemoryManager,
                HiCR::CommunicationManager        &coordinationCommunicationManager,
                HiCR::CommunicationManager        &payloadCommunicationManager,
                std::shared_ptr<HiCR::MemorySpace> coordinationMemorySpace,
                std::shared_ptr<HiCR::MemorySpace> payloadMemorySpace,
                const size_t                       channelCapacity,
                const unsigned int                 producerId)
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
  coordinationCommunicationManager.exchangeGlobalMemorySlots(CHANNEL_TAG, {});
  payloadCommunicationManager.exchangeGlobalMemorySlots(CHANNEL_TAG, {});

  // Synchronizing so that all actors have finished registering their global memory slots
  coordinationCommunicationManager.fence(CHANNEL_TAG);
  payloadCommunicationManager.fence(CHANNEL_TAG);

  // Obtaining the globally exchanged memory slots
  auto sizesBuffer                           = coordinationCommunicationManager.getGlobalMemorySlot(CHANNEL_TAG, SIZES_BUFFER_KEY);
  auto consumerCoordinationBufferForCounts   = coordinationCommunicationManager.getGlobalMemorySlot(CHANNEL_TAG, CONSUMER_COORDINATION_BUFFER_FOR_SIZES_KEY);
  auto consumerCoordinationBufferForPayloads = coordinationCommunicationManager.getGlobalMemorySlot(CHANNEL_TAG, CONSUMER_COORDINATION_BUFFER_FOR_PAYLOADS_KEY);
  auto payloadBuffer                         = payloadCommunicationManager.getGlobalMemorySlot(CHANNEL_TAG, CONSUMER_PAYLOAD_KEY);

  const size_t payloadCapacity = channelCapacity * sizeof(ELEMENT_TYPE);
  // Creating producer and consumer channels
  auto producer = HiCR::channel::variableSize::MPSC::locking::Producer(coordinationCommunicationManager,
                                                                       payloadCommunicationManager,
                                                                       sizeInfoBuffer,
                                                                       payloadBuffer,
                                                                       sizesBuffer,
                                                                       coordinationBufferForCounts,
                                                                       coordinationBufferForPayloads,
                                                                       consumerCoordinationBufferForCounts,
                                                                       consumerCoordinationBufferForPayloads,
                                                                       payloadCapacity,
                                                                       sizeof(ELEMENT_TYPE),
                                                                       channelCapacity);

  // Allocating a send slot to put the values we want to communicate
  ELEMENT_TYPE sendBuffer1[5] = {1u * producerId, 0u * producerId, 1u * producerId, 2u * producerId, 3u * producerId};
  ELEMENT_TYPE sendBuffer2[4] = {1u * producerId, 4u * producerId, 5u * producerId, 6u * producerId};
  ELEMENT_TYPE sendBuffer3[3] = {1u * producerId, 7u * producerId, 8u * producerId};
  auto         sendBuffer1Ptr = &sendBuffer1;
  auto         sendBuffer2Ptr = &sendBuffer2;
  auto         sendBuffer3Ptr = &sendBuffer3;
  auto         sendSlot1      = payloadMemoryManager.registerLocalMemorySlot(payloadMemorySpace, sendBuffer1Ptr, sizeof(sendBuffer1));
  auto         sendSlot2      = payloadMemoryManager.registerLocalMemorySlot(payloadMemorySpace, sendBuffer2Ptr, sizeof(sendBuffer2));
  auto         sendSlot3      = payloadMemoryManager.registerLocalMemorySlot(payloadMemorySpace, sendBuffer3Ptr, sizeof(sendBuffer3));

  // Pushing first batch
  char prefix[64] = {'\0'};
  sprintf(prefix, "PRODUCER %u sent:", producerId);
  while (producer.push(sendSlot1) == false);
  Printer<ELEMENT_TYPE>::printBytes(prefix, sendBuffer1Ptr, sizeof(sendBuffer1), 0, sizeof(sendBuffer1));

  // Sending second batch
  while (producer.push(sendSlot2) == false);
  Printer<ELEMENT_TYPE>::printBytes(prefix, sendBuffer2Ptr, sizeof(sendBuffer2), 0, sizeof(sendBuffer2));

  // Sending third batch (same as first batch)
  while (producer.push(sendSlot3) == false);
  Printer<ELEMENT_TYPE>::printBytes(prefix, sendBuffer3Ptr, sizeof(sendBuffer3), 0, sizeof(sendBuffer3));

  // Synchronizing so that all actors have finished registering their global memory slots
  coordinationCommunicationManager.fence(CHANNEL_TAG);
  payloadCommunicationManager.fence(CHANNEL_TAG);

  // De-registering global slots
  coordinationCommunicationManager.deregisterGlobalMemorySlot(sizesBuffer);
  coordinationCommunicationManager.deregisterGlobalMemorySlot(consumerCoordinationBufferForCounts);
  coordinationCommunicationManager.deregisterGlobalMemorySlot(consumerCoordinationBufferForPayloads);

  // Fence for Global Slot destruction/cleanup
  coordinationCommunicationManager.fence(CHANNEL_TAG);
  payloadCommunicationManager.fence(CHANNEL_TAG);

  // Freeing up local memory
  coordinationMemoryManager.freeLocalMemorySlot(coordinationBufferForCounts);
  coordinationMemoryManager.freeLocalMemorySlot(coordinationBufferForPayloads);
  coordinationMemoryManager.freeLocalMemorySlot(sizeInfoBuffer);
}
