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

#include <gtest/gtest.h>
#include <hicr/core/memoryManager.hpp>
#include <hicr/core/communicationManager.hpp>
#include <hicr/frontends/channel/variableSize/spsc/producer.hpp>

#include "common.hpp"

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
                                                              CHANNEL_CAPACITY * sizeof(ELEMENT_TYPE),
                                                              sizeof(ELEMENT_TYPE),
                                                              channelCapacity);

  ////////////////////// Test begin

  // Send a buffer big as the buffer channel
  ELEMENT_TYPE sendBuffer[CHANNEL_CAPACITY] = {0, 1, 2, 3, 4};
  ELEMENT_TYPE sendBuffer2[1]               = {0};
  auto         sendBufferPtr                = &sendBuffer;
  auto         sendBuffer2Ptr               = &sendBuffer;
  auto         sendSlot                     = payloadMemoryManager.registerLocalMemorySlot(payloadMemorySpace, sendBufferPtr, sizeof(sendBuffer));
  auto         sendSlot2                    = payloadMemoryManager.registerLocalMemorySlot(payloadMemorySpace, sendBuffer2Ptr, sizeof(sendBuffer2));

  // Wait for the consumer 1
  coordinationCommunicationManager.fence(CHANNEL_TAG);
  payloadCommunicationManager.fence(CHANNEL_TAG);

  // Pushing first batch that fills the channel
  ASSERT_EQ(producer.getPayloadCapacity(), CHANNEL_CAPACITY * sizeof(ELEMENT_TYPE));
  ASSERT_TRUE(producer.isEmpty());
  ASSERT_FALSE(producer.isFull(0));
  producer.updateDepth();
  ASSERT_EQ(producer.getCoordinationDepth(), 0);
  ASSERT_EQ(producer.getPayloadDepth(), 0);
  EXPECT_NO_THROW(producer.push(sendSlot));
  ASSERT_TRUE(producer.isFull(0));
  producer.updateDepth();
  ASSERT_EQ(producer.getCoordinationDepth(), 1);
  ASSERT_EQ(producer.getPayloadDepth(), CHANNEL_CAPACITY * sizeof(ELEMENT_TYPE));
  EXPECT_THROW(producer.push(sendSlot2), HiCR::RuntimeException);

  // Wait for the consumer 2
  coordinationCommunicationManager.fence(CHANNEL_TAG);
  payloadCommunicationManager.fence(CHANNEL_TAG);

  // Wait for the consumer 3
  coordinationCommunicationManager.fence(CHANNEL_TAG);
  payloadCommunicationManager.fence(CHANNEL_TAG);

  // Send token one by one
  for (size_t i = 0; i < CHANNEL_CAPACITY; ++i)
  {
    ELEMENT_TYPE sendBuffer[1] = {0};
    auto         sendBufferPtr = &sendBuffer;
    auto         sendSlot      = payloadMemoryManager.registerLocalMemorySlot(payloadMemorySpace, sendBufferPtr, sizeof(sendBuffer));
    ASSERT_EQ(producer.getCoordinationDepth(), i);
    ASSERT_EQ(producer.getPayloadDepth(), i * sizeof(ELEMENT_TYPE));
    ASSERT_NO_THROW(producer.push(sendSlot));
    coordinationCommunicationManager.fence(CHANNEL_TAG);
    payloadCommunicationManager.fence(CHANNEL_TAG);
    ASSERT_EQ(producer.getCoordinationDepth(), i + 1);
    ASSERT_EQ(producer.getPayloadDepth(), (i + 1) * sizeof(ELEMENT_TYPE));
  }

  ASSERT_TRUE(producer.isFull(0));

  // Wait for the consumer 4
  coordinationCommunicationManager.fence(CHANNEL_TAG);
  payloadCommunicationManager.fence(CHANNEL_TAG);

  for (size_t i = CHANNEL_CAPACITY; i > 0; --i)
  {
    coordinationCommunicationManager.fence(CHANNEL_TAG);
    payloadCommunicationManager.fence(CHANNEL_TAG);
    ASSERT_EQ(producer.getCoordinationDepth(), i - 1);
    ASSERT_EQ(producer.getPayloadDepth(), (i - 1) * sizeof(ELEMENT_TYPE));
  }

  // Wait for the consumer 5
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
