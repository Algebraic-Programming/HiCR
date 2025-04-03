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

#include <hicr/frontends/channel/variableSize/spsc/consumer.hpp>
#include "common.hpp"

void reader(HiCR::L1::MemoryManager &memoryManager, HiCR::L1::CommunicationManager &communicationManager, HiCR::objectStore::ObjectStore &objectStore)
{
  auto memorySpace = objectStore.getMemorySpace();
  // BEGIN Channel initialization phase

  // Getting required buffer sizes
  auto sizesBufferSize = HiCR::channel::variableSize::Base::getTokenBufferSize(sizeof(uint8_t), CHANNEL_PAYLOAD_CAPACITY);

  // Allocating sizes buffer as a local memory slot
  auto sizesBufferSlot = memoryManager.allocateLocalMemorySlot(memorySpace, sizesBufferSize);

  // Allocating payload buffer as a local memory slot
  auto payloadBufferSlot = memoryManager.allocateLocalMemorySlot(memorySpace, CHANNEL_PAYLOAD_CAPACITY);

  // Getting required buffer size
  auto coordinationBufferSize = HiCR::channel::variableSize::Base::getCoordinationBufferSize();

  // Allocating coordination buffer for internal message size metadata
  auto coordinationBufferForCounts = memoryManager.allocateLocalMemorySlot(memorySpace, coordinationBufferSize);

  // Allocating coordination buffer for internal payload metadata
  auto coordinationBufferForPayloads = memoryManager.allocateLocalMemorySlot(memorySpace, coordinationBufferSize);

  // Initializing coordination buffer (sets to zero the counters)
  HiCR::channel::variableSize::Base::initializeCoordinationBuffer(coordinationBufferForCounts);

  HiCR::channel::variableSize::Base::initializeCoordinationBuffer(coordinationBufferForPayloads);

  // Exchanging local memory slots to become global for them to be used by the remote end
  communicationManager.exchangeGlobalMemorySlots(CHANNEL_TAG,
                                                 {{SIZES_BUFFER_KEY, sizesBufferSlot},
                                                  {CONSUMER_COORDINATION_BUFFER_FOR_SIZES_KEY, coordinationBufferForCounts},
                                                  {CONSUMER_COORDINATION_BUFFER_FOR_PAYLOADS_KEY, coordinationBufferForPayloads},
                                                  {CONSUMER_PAYLOAD_KEY, payloadBufferSlot}});

  // Synchronizing so that all actors have finished registering their global memory slots
  communicationManager.fence(CHANNEL_TAG);

  // Obtaining the globally exchanged memory slots
  std::shared_ptr<HiCR::L0::GlobalMemorySlot> globalSizesBufferSlot = communicationManager.getGlobalMemorySlot(CHANNEL_TAG, SIZES_BUFFER_KEY);

  auto producerCoordinationBufferForCounts   = communicationManager.getGlobalMemorySlot(CHANNEL_TAG, PRODUCER_COORDINATION_BUFFER_FOR_SIZES_KEY);
  auto producerCoordinationBufferForPayloads = communicationManager.getGlobalMemorySlot(CHANNEL_TAG, PRODUCER_COORDINATION_BUFFER_FOR_PAYLOADS_KEY);
  auto consumerCoordinationBufferForCounts   = communicationManager.getGlobalMemorySlot(CHANNEL_TAG, CONSUMER_COORDINATION_BUFFER_FOR_SIZES_KEY);
  auto consumerCoordinationBufferForPayloads = communicationManager.getGlobalMemorySlot(CHANNEL_TAG, CONSUMER_COORDINATION_BUFFER_FOR_PAYLOADS_KEY);
  auto payloadBuffer                         = communicationManager.getGlobalMemorySlot(CHANNEL_TAG, CONSUMER_PAYLOAD_KEY);

  // Creating producer and consumer channels
  auto consumer = HiCR::channel::variableSize::SPSC::Consumer(communicationManager,
                                                              payloadBuffer /*payload buffer */,
                                                              globalSizesBufferSlot,
                                                              coordinationBufferForCounts,
                                                              coordinationBufferForPayloads,
                                                              producerCoordinationBufferForCounts,
                                                              producerCoordinationBufferForPayloads,
                                                              CHANNEL_PAYLOAD_CAPACITY,
                                                              CHANNEL_COUNT_CAPACITY);

  // END Channel initialization phase

  // To deserialize the messages
  HiCR::objectStore::handle handle1;
  HiCR::objectStore::handle handle2;

  // Wait until you receive a message
  while (consumer.isEmpty()) consumer.updateDepth();

  // Getting internal pointer of the token buffer slot
  auto payloadBufferPtr = payloadBufferSlot->getPointer();

  auto res = consumer.peek(); // res is a 2-element array containing the index and size of the message

  // Handle unexpected message size; this should not happen as we strictly expect a handle
  if (res[1] != sizeof(HiCR::objectStore::handle))
  {
    printf("Reader: Received block 1: %hhn\n", (uint8_t *)payloadBufferPtr + res[0]);
    return;
  }
  std::memcpy(&handle1, (uint8_t *)payloadBufferPtr + res[0], sizeof(HiCR::objectStore::handle));

  // Pop the message
  consumer.pop();

  // Deserialize the handle
  auto dataObject1 = objectStore.deserialize(handle1);

  auto objSlot1 = objectStore.get(*dataObject1);

  // One-sided fence to ensure this block is received
  objectStore.fence(dataObject1);

  printf("Reader: Received block 1: %s\n", (char *)objSlot1->getPointer());

  // Wait until you receive a message
  while (consumer.isEmpty()) consumer.updateDepth();
  res = consumer.peek();
  // uint8_t *serializedBlock2 = new uint8_t[res[1]];
  // std::memcpy(serializedBlock2, payloadBufferPtr[res[0]], res[1]);
  if (res[1] != sizeof(HiCR::objectStore::handle))
  {
    printf("Reader: Received block 2: %hhn\n", (uint8_t *)payloadBufferPtr + res[0]);
    return;
  }
  std::memcpy(&handle2, (uint8_t *)payloadBufferPtr + res[0], sizeof(HiCR::objectStore::handle));

  // Pop the message
  consumer.pop();

  communicationManager.fence(CHANNEL_TAG);

  // Deserialize the handle
  auto dataObject2 = objectStore.deserialize(handle2);

  auto objSlot2 = objectStore.get(*dataObject2);

  // Fence to ensure all blocks are received
  objectStore.fence();

  printf("Reader: Received block 2: %s\n", (char *)objSlot2->getPointer());

  objectStore.destroy(*dataObject1);
  objectStore.destroy(*dataObject2);

  // Clean up channel
  communicationManager.deregisterGlobalMemorySlot(globalSizesBufferSlot);
  communicationManager.deregisterGlobalMemorySlot(producerCoordinationBufferForCounts);
  communicationManager.deregisterGlobalMemorySlot(producerCoordinationBufferForPayloads);
  communicationManager.deregisterGlobalMemorySlot(consumerCoordinationBufferForCounts);
  communicationManager.deregisterGlobalMemorySlot(consumerCoordinationBufferForPayloads);

  communicationManager.destroyGlobalMemorySlot(consumerCoordinationBufferForCounts);
  communicationManager.destroyGlobalMemorySlot(consumerCoordinationBufferForPayloads);
  communicationManager.destroyGlobalMemorySlot(payloadBuffer);

  communicationManager.fence(CHANNEL_TAG);

  memoryManager.freeLocalMemorySlot(coordinationBufferForCounts);
  memoryManager.freeLocalMemorySlot(coordinationBufferForPayloads);
  memoryManager.freeLocalMemorySlot(sizesBufferSlot);
  memoryManager.freeLocalMemorySlot(payloadBufferSlot);
}
