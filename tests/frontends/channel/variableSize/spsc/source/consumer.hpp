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
#include <hicr/frontends/channel/variableSize/spsc/consumer.hpp>
#include "common.hpp"

__INLINE__ std::shared_ptr<HiCR::LocalMemorySlot> peek(HiCR::channel::variableSize::SPSC::Consumer &consumerInterface,
                                                       HiCR::MemoryManager                         &memoryManager,
                                                       std::shared_ptr<HiCR::MemorySpace>          &memorySpace)
{
  // If the buffer is full, returning false
  while (consumerInterface.isEmpty()) {}

  // Pushing buffer
  auto result = consumerInterface.peek();

  // Getting absolute pointer to the token
  size_t tokenPos    = result[0];
  size_t tokenSize   = result[1];
  auto   tokenBuffer = (uint8_t *)consumerInterface.getPayloadBufferMemorySlot()->getSourceLocalMemorySlot()->getPointer();
  void  *tokenPtr    = &tokenBuffer[tokenPos];

  // Register and return the memory slot
  return memoryManager.registerLocalMemorySlot(memorySpace, tokenPtr, tokenSize);
}

void consumerFc(HiCR::MemoryManager               &coordinationMemoryManager,
                HiCR::MemoryManager               &payloadMemoryManager,
                HiCR::CommunicationManager        &coordinationCommunicationManager,
                HiCR::CommunicationManager        &payloadCommunicationManager,
                std::shared_ptr<HiCR::MemorySpace> coordinationMemorySpace,
                std::shared_ptr<HiCR::MemorySpace> payloadMemorySpace,
                const size_t                       channelCapacity)
{
  // Getting required buffer sizes
  auto sizesBufferSize = HiCR::channel::variableSize::Base::getTokenBufferSize(sizeof(size_t), channelCapacity);

  // Allocating sizes buffer as a local memory slot
  auto sizesBufferSlot = coordinationMemoryManager.allocateLocalMemorySlot(coordinationMemorySpace, sizesBufferSize);

  // Allocating payload buffer as a local memory slot
  auto payloadBufferSlot = payloadMemoryManager.allocateLocalMemorySlot(payloadMemorySpace, 2 * CHANNEL_CAPACITY * sizeof(ELEMENT_TYPE));

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
  std::shared_ptr<HiCR::GlobalMemorySlot> globalSizesBufferSlot = coordinationCommunicationManager.getGlobalMemorySlot(CHANNEL_TAG, SIZES_BUFFER_KEY);

  auto producerCoordinationBufferForCounts   = coordinationCommunicationManager.getGlobalMemorySlot(CHANNEL_TAG, PRODUCER_COORDINATION_BUFFER_FOR_SIZES_KEY);
  auto producerCoordinationBufferForPayloads = coordinationCommunicationManager.getGlobalMemorySlot(CHANNEL_TAG, PRODUCER_COORDINATION_BUFFER_FOR_PAYLOADS_KEY);
  auto consumerCoordinationBufferForCounts   = coordinationCommunicationManager.getGlobalMemorySlot(CHANNEL_TAG, CONSUMER_COORDINATION_BUFFER_FOR_SIZES_KEY);
  auto consumerCoordinationBufferForPayloads = coordinationCommunicationManager.getGlobalMemorySlot(CHANNEL_TAG, CONSUMER_COORDINATION_BUFFER_FOR_PAYLOADS_KEY);
  auto payloadBuffer                         = payloadCommunicationManager.getGlobalMemorySlot(CHANNEL_TAG, CONSUMER_PAYLOAD_KEY);

  // Creating producer and consumer channels
  auto consumer = HiCR::channel::variableSize::SPSC::Consumer(coordinationCommunicationManager,
                                                              payloadCommunicationManager,
                                                              payloadBuffer /*payload buffer */,
                                                              globalSizesBufferSlot,
                                                              coordinationBufferForCounts,
                                                              coordinationBufferForPayloads,
                                                              producerCoordinationBufferForCounts,
                                                              producerCoordinationBufferForPayloads,
                                                              CHANNEL_CAPACITY * sizeof(ELEMENT_TYPE),
                                                              channelCapacity);

  ASSERT_TRUE(consumer.isEmpty());
  ASSERT_EQ(consumer.getCoordinationDepth(), 0);
  ASSERT_EQ(consumer.getPayloadDepth(), 0);
  ASSERT_FALSE(consumer.isFull(CHANNEL_CAPACITY * sizeof(ELEMENT_TYPE)));
  ASSERT_TRUE(consumer.isFull(CHANNEL_CAPACITY * sizeof(ELEMENT_TYPE) + 1));

  // Wait for producer 1
  coordinationCommunicationManager.fence(CHANNEL_TAG);
  payloadCommunicationManager.fence(CHANNEL_TAG);

  // Wait for the producer 2
  coordinationCommunicationManager.fence(CHANNEL_TAG);
  payloadCommunicationManager.fence(CHANNEL_TAG);

  consumer.updateDepth();
  ASSERT_EQ(consumer.getCoordinationDepth(), 1);
  ASSERT_EQ(consumer.getPayloadDepth(), CHANNEL_CAPACITY * sizeof(ELEMENT_TYPE));

  // Peek
  auto res = consumer.peek();
  ASSERT_EQ(res[0], 0);
  ASSERT_EQ(res[1], CHANNEL_CAPACITY * sizeof(ELEMENT_TYPE));

  auto  tokenBuffer = (uint8_t *)consumer.getPayloadBufferMemorySlot()->getSourceLocalMemorySlot()->getPointer();
  void *tokenPtr    = &tokenBuffer[res[0]];
  for (ELEMENT_TYPE i = 0; i < (res[1] / sizeof(ELEMENT_TYPE)); ++i) { ASSERT_EQ(i, static_cast<ELEMENT_TYPE *>(tokenPtr)[i]); }

  consumer.pop();
  ASSERT_TRUE(consumer.isEmpty());
  ASSERT_EQ(consumer.getCoordinationDepth(), 0);
  ASSERT_EQ(consumer.getPayloadDepth(), 0);

  // Wait for the producer 3
  coordinationCommunicationManager.fence(CHANNEL_TAG);
  payloadCommunicationManager.fence(CHANNEL_TAG);

  // Send token one by one
  for (size_t i = 0; i < CHANNEL_CAPACITY; ++i)
  {
    coordinationCommunicationManager.fence(CHANNEL_TAG);
    payloadCommunicationManager.fence(CHANNEL_TAG);

    ASSERT_EQ(consumer.getCoordinationDepth(), i + 1);
    ASSERT_EQ(consumer.getPayloadDepth(), (i + 1) * sizeof(ELEMENT_TYPE));
  }

  ASSERT_TRUE(consumer.isFull(0));

  // Wait for the producer 4
  coordinationCommunicationManager.fence(CHANNEL_TAG);
  payloadCommunicationManager.fence(CHANNEL_TAG);

  // Pop token one by one
  auto peekIndex = 0;
  for (size_t i = CHANNEL_CAPACITY; i > 0; --i)
  {
    ASSERT_EQ(consumer.getCoordinationDepth(), i);
    ASSERT_EQ(consumer.getPayloadDepth(), i * sizeof(ELEMENT_TYPE));
    auto res = consumer.peek();
    ASSERT_EQ(res[0], peekIndex++ * sizeof(ELEMENT_TYPE));
    ASSERT_EQ(res[1], sizeof(ELEMENT_TYPE));

    auto  tokenBuffer = (uint8_t *)consumer.getPayloadBufferMemorySlot()->getSourceLocalMemorySlot()->getPointer();
    void *tokenPtr    = &tokenBuffer[res[0]];
    auto  token       = static_cast<ELEMENT_TYPE *>(tokenPtr)[0];
    for (ELEMENT_TYPE i = 0; i < CHANNEL_CAPACITY; ++i) { ASSERT_EQ(0, token); }
    consumer.pop();
    coordinationCommunicationManager.fence(CHANNEL_TAG);
    payloadCommunicationManager.fence(CHANNEL_TAG);
    ASSERT_EQ(consumer.getCoordinationDepth(), i - 1);
    ASSERT_EQ(consumer.getPayloadDepth(), (i - 1) * sizeof(ELEMENT_TYPE));
  }

  // Wait for the producer 5
  coordinationCommunicationManager.fence(CHANNEL_TAG);
  payloadCommunicationManager.fence(CHANNEL_TAG);

  // De-registering global slots
  coordinationCommunicationManager.deregisterGlobalMemorySlot(globalSizesBufferSlot);
  coordinationCommunicationManager.deregisterGlobalMemorySlot(producerCoordinationBufferForCounts);
  coordinationCommunicationManager.deregisterGlobalMemorySlot(producerCoordinationBufferForPayloads);
  coordinationCommunicationManager.deregisterGlobalMemorySlot(consumerCoordinationBufferForCounts);
  coordinationCommunicationManager.deregisterGlobalMemorySlot(consumerCoordinationBufferForPayloads);

  // Destroying global slots (collective calls)
  coordinationCommunicationManager.destroyGlobalMemorySlot(consumerCoordinationBufferForCounts);
  coordinationCommunicationManager.destroyGlobalMemorySlot(consumerCoordinationBufferForPayloads);
  payloadCommunicationManager.destroyGlobalMemorySlot(payloadBuffer);

  coordinationCommunicationManager.fence(CHANNEL_TAG);
  payloadCommunicationManager.fence(CHANNEL_TAG);

  // Freeing up local memory
  payloadMemoryManager.freeLocalMemorySlot(payloadBufferSlot);
  coordinationMemoryManager.freeLocalMemorySlot(sizesBufferSlot);
  coordinationMemoryManager.freeLocalMemorySlot(coordinationBufferForCounts);
  coordinationMemoryManager.freeLocalMemorySlot(coordinationBufferForPayloads);
}
