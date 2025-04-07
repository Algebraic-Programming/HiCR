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

#include <hicr/frontends/channel/variableSize/spsc/producer.hpp>
#include "common.hpp"

void owner(HiCR::MemoryManager &memoryManager, HiCR::CommunicationManager &communicationManager, HiCR::objectStore::ObjectStore &objectStore)
{
  auto memorySpace = objectStore.getMemorySpace();
  // BEGIN Channel initialization phase

  // Getting required buffer size
  auto coordinationBufferSize = HiCR::channel::variableSize::Base::getCoordinationBufferSize();

  // Allocating sizes buffer as a local memory slot
  auto coordinationBufferForCounts = memoryManager.allocateLocalMemorySlot(memorySpace, coordinationBufferSize);

  auto coordinationBufferForPayloads = memoryManager.allocateLocalMemorySlot(memorySpace, coordinationBufferSize);

  auto sizeInfoBuffer = memoryManager.allocateLocalMemorySlot(memorySpace, sizeof(size_t));

  // Initializing coordination buffers for message sizes and payloads (sets to zero the counters)
  HiCR::channel::variableSize::Base::initializeCoordinationBuffer(coordinationBufferForCounts);
  HiCR::channel::variableSize::Base::initializeCoordinationBuffer(coordinationBufferForPayloads);

  // Exchanging local memory slots to become global for them to be used by the remote end
  communicationManager.exchangeGlobalMemorySlots(CHANNEL_TAG,                                                                /* global tag */
                                                 {{PRODUCER_COORDINATION_BUFFER_FOR_SIZES_KEY, coordinationBufferForCounts}, /* key-slot pairs */
                                                  {PRODUCER_COORDINATION_BUFFER_FOR_PAYLOADS_KEY, coordinationBufferForPayloads}});

  // Synchronizing so that all actors have finished registering their global memory slots
  communicationManager.fence(CHANNEL_TAG);

  // Obtaining the globally exchanged memory slots
  auto sizesBuffer                           = communicationManager.getGlobalMemorySlot(CHANNEL_TAG, SIZES_BUFFER_KEY);
  auto producerCoordinationBufferForCounts   = communicationManager.getGlobalMemorySlot(CHANNEL_TAG, PRODUCER_COORDINATION_BUFFER_FOR_SIZES_KEY);
  auto producerCoordinationBufferForPayloads = communicationManager.getGlobalMemorySlot(CHANNEL_TAG, PRODUCER_COORDINATION_BUFFER_FOR_PAYLOADS_KEY);
  auto consumerCoordinationBufferForCounts   = communicationManager.getGlobalMemorySlot(CHANNEL_TAG, CONSUMER_COORDINATION_BUFFER_FOR_SIZES_KEY);
  auto consumerCoordinationBufferForPayloads = communicationManager.getGlobalMemorySlot(CHANNEL_TAG, CONSUMER_COORDINATION_BUFFER_FOR_PAYLOADS_KEY);
  auto payloadBuffer                         = communicationManager.getGlobalMemorySlot(CHANNEL_TAG, CONSUMER_PAYLOAD_KEY);

  // Creating producer and consumer channels
  auto producer = HiCR::channel::variableSize::SPSC::Producer(communicationManager,
                                                              sizeInfoBuffer,
                                                              payloadBuffer,
                                                              sizesBuffer,
                                                              coordinationBufferForCounts,
                                                              coordinationBufferForPayloads,
                                                              consumerCoordinationBufferForCounts,
                                                              consumerCoordinationBufferForPayloads,
                                                              CHANNEL_PAYLOAD_CAPACITY,
                                                              sizeof(uint8_t),
                                                              CHANNEL_COUNT_CAPACITY);

  // END Channel initialization phase

  // Allocate memory for our blocks
  auto myBlockSlot  = memoryManager.allocateLocalMemorySlot(objectStore.getMemorySpace(), BLOCK_SIZE);
  auto myBlockSlot2 = memoryManager.allocateLocalMemorySlot(objectStore.getMemorySpace(), BLOCK_SIZE);

  char *myBlockData  = (char *)myBlockSlot->getPointer();
  char *myBlockData2 = (char *)myBlockSlot2->getPointer();

  auto myBlock  = objectStore.createObject(myBlockSlot, 0);
  auto myBlock2 = objectStore.createObject(myBlockSlot2, 1);

  // Initialize the block with "Test"
  myBlockData[0] = 'T';
  myBlockData[1] = 'e';
  myBlockData[2] = 's';
  myBlockData[3] = 't';
  myBlockData[4] = '\0';

  // Publish the first block
  objectStore.publish(myBlock);

  myBlockData2[0]  = 'T';
  myBlockData2[1]  = 'h';
  myBlockData2[2]  = 'i';
  myBlockData2[3]  = 's';
  myBlockData2[4]  = ' ';
  myBlockData2[5]  = 'i';
  myBlockData2[6]  = 's';
  myBlockData2[7]  = ' ';
  myBlockData2[8]  = 'a';
  myBlockData2[9]  = 'n';
  myBlockData2[10] = 'o';
  myBlockData2[11] = 't';
  myBlockData2[12] = 'h';
  myBlockData2[13] = 'e';
  myBlockData2[14] = 'r';
  myBlockData2[15] = ' ';
  myBlockData2[16] = 'b';
  myBlockData2[17] = 'l';
  myBlockData2[18] = 'o';
  myBlockData2[19] = 'c';
  myBlockData2[20] = 'k';
  myBlockData2[21] = '\0';

  // Publish the second block
  objectStore.publish(myBlock2);

  // Serialize the blocks
  auto handle1 = objectStore.serialize(*myBlock);
  auto handle2 = objectStore.serialize(*myBlock2);

  // Create buffers to send the serialized blocks, copy the serialized blocks into the buffers
  uint8_t *serializedBlock  = new uint8_t[sizeof(HiCR::objectStore::handle)];
  uint8_t *serializedBlock2 = new uint8_t[sizeof(HiCR::objectStore::handle)];

  std::memcpy(serializedBlock, &handle1, sizeof(HiCR::objectStore::handle));
  std::memcpy(serializedBlock2, &handle2, sizeof(HiCR::objectStore::handle));

  auto sendSlot  = memoryManager.registerLocalMemorySlot(memorySpace, serializedBlock, sizeof(HiCR::objectStore::handle));
  auto sendSlot2 = memoryManager.registerLocalMemorySlot(memorySpace, serializedBlock2, sizeof(HiCR::objectStore::handle));

  // Send the block information to the reader via the channel
  producer.push(sendSlot);

  while (!producer.isEmpty()) producer.updateDepth();

  // Send the second block information to the reader via the channel
  producer.push(sendSlot2);

  // Fence to ensure all blocks are sent
  communicationManager.fence(CHANNEL_TAG);

  // Delete the sent buffers
  delete[] serializedBlock;
  delete[] serializedBlock2;

  // Fence all pending gets from the reader side
  objectStore.fence();

  // Delete the block
  objectStore.destroy(*myBlock);
  objectStore.destroy(*myBlock2);

  // Free the memory slot
  memoryManager.freeLocalMemorySlot(myBlockSlot);
  memoryManager.freeLocalMemorySlot(myBlockSlot2);

  // Channel cleanup
  communicationManager.deregisterGlobalMemorySlot(sizesBuffer);
  communicationManager.deregisterGlobalMemorySlot(producerCoordinationBufferForCounts);
  communicationManager.deregisterGlobalMemorySlot(producerCoordinationBufferForPayloads);

  communicationManager.destroyGlobalMemorySlot(producerCoordinationBufferForCounts);
  communicationManager.destroyGlobalMemorySlot(producerCoordinationBufferForPayloads);

  communicationManager.fence(CHANNEL_TAG);

  memoryManager.freeLocalMemorySlot(coordinationBufferForCounts);
  memoryManager.freeLocalMemorySlot(coordinationBufferForPayloads);
  memoryManager.freeLocalMemorySlot(sizeInfoBuffer);
}
