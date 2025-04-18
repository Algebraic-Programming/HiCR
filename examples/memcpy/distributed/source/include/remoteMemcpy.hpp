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

#include <cstdio>
#include <hicr/core/instanceManager.hpp>
#include <hicr/core/memoryManager.hpp>
#include <hicr/core/communicationManager.hpp>
#include <hicr/core/topologyManager.hpp>

#define BUFFER_SIZE 256
#define DST_OFFSET 0
#define SRC_OFFSET 0
#define COMM_TAG 0

void remoteMemcpy(HiCR::InstanceManager      *instanceManager,
                  HiCR::TopologyManager      *topologyManager,
                  HiCR::MemoryManager        *memoryManager,
                  HiCR::CommunicationManager *communicationManager)
{
  // Getting my current HiCR instance
  auto myInstance = instanceManager->getCurrentInstance();

  // Getting current process id
  auto myInstanceId = instanceManager->getCurrentInstance()->getId();

  // Getting root HiCR instance id
  auto rootInstanceId = instanceManager->getRootInstanceId();

  // Getting HiCR instance list
  auto instanceList = instanceManager->getInstances();

  // Making sure only two HiCR instances are involved in the execution of this example
  if (instanceList.size() != 2)
  {
    if (myInstance->getId() == rootInstanceId) fprintf(stderr, "[Error] This example requires exactly 2 HiCR instances to run\n");
    instanceManager->abort(1);
  }

  // Determining receiver and sender instance ids
  auto                         senderId   = rootInstanceId;
  HiCR::Instance::instanceId_t receiverId = 0;
  for (const auto &instance : instanceList)
    if (instance->getId() != senderId) receiverId = instance->getId();

  // Asking backend to check the available devices
  const auto topology = topologyManager->queryTopology();

  // Getting first device found
  auto device = *topology.getDevices().begin();

  // Obtaining memory spaces
  auto memSpaces = device->getMemorySpaceList();

  // Getting the first memory space we found
  auto firstMemSpace = *memSpaces.begin();

  // Allocating send/receive buffer
  auto bufferSlot = memoryManager->allocateLocalMemorySlot(firstMemSpace, BUFFER_SIZE);

  // Performing memory slot exchange now
  if (myInstanceId == senderId) communicationManager->exchangeGlobalMemorySlots(COMM_TAG, {});
  if (myInstanceId == receiverId) communicationManager->exchangeGlobalMemorySlots(COMM_TAG, {{myInstanceId, bufferSlot}});

  // Synchronizing so that all actors have finished registering their global memory slots
  communicationManager->fence(COMM_TAG);

  // Getting remote memory slot from receiver
  auto receiverSlot = communicationManager->getGlobalMemorySlot(COMM_TAG, receiverId);

  // If I am the sender, write a message and send it
  if (myInstanceId == senderId)
  {
    // Writing message
    sprintf((char *)bufferSlot->getPointer(), "Hello, receiver! This is sender.");

    // Sending message to the receiver (put)
    communicationManager->memcpy(receiverSlot, DST_OFFSET, bufferSlot, SRC_OFFSET, BUFFER_SIZE);

    // Fencing (waiting) on the first operation to complete
    communicationManager->fence(COMM_TAG);

    // Retrieving message to the receiver (get)
    communicationManager->memcpy(bufferSlot, DST_OFFSET, receiverSlot, SRC_OFFSET, BUFFER_SIZE);

    // Fencing (waiting) on the second operation to complete
    communicationManager->fence(COMM_TAG);

    // Printing buffer
    printf("[Sender] Received buffer: %s\n", (const char *)bufferSlot->getPointer());
  }

  // If I am the receiver, wait for the message to arrive and then print it
  if (myInstanceId == receiverId)
  {
    // Fencing (waiting) on the operation to complete
    communicationManager->fence(COMM_TAG);

    // Querying number of messages received
    communicationManager->queryMemorySlotUpdates(receiverSlot->getSourceLocalMemorySlot());
    auto recvMsgs = receiverSlot->getSourceLocalMemorySlot()->getMessagesRecv();

    // Printinf number of messages received
    printf("[Receiver] Received Message Count: %lu\n", recvMsgs);

    // Printing buffer
    printf("[Receiver] Received buffer: %s\n", (const char *)bufferSlot->getPointer());

    // Writing second message message
    sprintf((char *)bufferSlot->getPointer(), "Hello, sender! This is receiver.");

    // Fencing (waiting) on the second operation to complete
    communicationManager->fence(COMM_TAG);
  }

  // De-registering global slots (collective call)
  communicationManager->deregisterGlobalMemorySlot(receiverSlot);
  communicationManager->destroyGlobalMemorySlot(receiverSlot);

  communicationManager->fence(COMM_TAG);

  // Freeing up local buffer
  memoryManager->freeLocalMemorySlot(bufferSlot);
}
