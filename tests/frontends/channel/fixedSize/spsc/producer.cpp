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

/**
 * @file spsc/producer.cpp
 * @brief Unit tests for the SPSC Producer Channel class
 * @author S. M. Martin
 * @date 19/9/2023
 */

#include <thread>
#include "gtest/gtest.h"
#include <hicr/backends/host/hwloc/L1/memoryManager.hpp>
#include <hicr/backends/host/pthreads/L1/communicationManager.hpp>
#include <hicr/backends/host/hwloc/L1/topologyManager.hpp>
#include <hicr/frontends/channel/fixedSize/spsc/consumer.hpp>
#include <hicr/frontends/channel/fixedSize/spsc/producer.hpp>

#define CHANNEL_TAG 0
#define TOKEN_BUFFER_KEY 0
#define PRODUCER_COORDINATION_BUFFER_KEY 1
#define CONSUMER_COORDINATION_BUFFER_KEY 2
#define BAD_PRODUCER_COORDINATION_BUFFER_KEY 3

TEST(ProducerChannel, Construction)
{
  // Creating HWloc topology object
  hwloc_topology_t topology;

  // Reserving memory for hwloc
  hwloc_topology_init(&topology);

  // Instantiating HWloc-based host (CPU) memory manager
  HiCR::backend::host::hwloc::L1::MemoryManager m(&topology);

  // Instantiating Pthread-based host (CPU) communication manager
  HiCR::backend::host::pthreads::L1::CommunicationManager c(1);

  // Initializing HWLoc-based host (CPU) topology manager
  HiCR::backend::host::hwloc::L1::TopologyManager tm(&topology);

  // Asking backend to check the available devices
  const auto t = tm.queryTopology();

  // Getting first device found
  auto d = *t.getDevices().begin();

  // Obtaining memory spaces
  auto memSpaces = d->getMemorySpaceList();

  // Channel configuration
  const auto tokenSize       = sizeof(size_t);
  const auto channelCapacity = 16;

  // Getting required buffer sizes
  auto tokenBufferSize        = HiCR::channel::fixedSize::SPSC::Consumer::getTokenBufferSize(tokenSize, channelCapacity);
  auto coordinationBufferSize = HiCR::channel::fixedSize::SPSC::Producer::getCoordinationBufferSize();

  // Allocating bad memory slots
  auto badCoordinationBuffer = m.allocateLocalMemorySlot(*memSpaces.begin(), coordinationBufferSize - 1);

  // Allocating correct memory slots
  auto correctTokenBuffer                = m.allocateLocalMemorySlot(*memSpaces.begin(), tokenBufferSize);
  auto correctProducerCoordinationBuffer = m.allocateLocalMemorySlot(*memSpaces.begin(), coordinationBufferSize);
  auto consumerCoordinationBuffer        = m.allocateLocalMemorySlot(*memSpaces.begin(), coordinationBufferSize);

  // Exchanging local memory slots to become global for them to be used by the remote end
  c.exchangeGlobalMemorySlots(
    CHANNEL_TAG,
    {{TOKEN_BUFFER_KEY, correctTokenBuffer}, {PRODUCER_COORDINATION_BUFFER_KEY, correctProducerCoordinationBuffer}, {CONSUMER_COORDINATION_BUFFER_KEY, consumerCoordinationBuffer}});

  // Synchronizing so that all actors have finished registering their global memory slots
  c.fence(CHANNEL_TAG);

  // Obtaining the globally exchanged memory slots
  auto globalTokenBuffer                = c.getGlobalMemorySlot(CHANNEL_TAG, TOKEN_BUFFER_KEY);
  auto globalProducerCoordinationBuffer = c.getGlobalMemorySlot(CHANNEL_TAG, PRODUCER_COORDINATION_BUFFER_KEY);
  auto globalConsumerCoordinationBuffer = c.getGlobalMemorySlot(CHANNEL_TAG, CONSUMER_COORDINATION_BUFFER_KEY);

  // Creating with incorrect parameters
  EXPECT_THROW(new HiCR::channel::fixedSize::SPSC::Producer(c, globalTokenBuffer, badCoordinationBuffer, globalProducerCoordinationBuffer, tokenSize, channelCapacity),
               HiCR::LogicException);

  // Creating with correct parameters
  EXPECT_NO_THROW(
    new HiCR::channel::fixedSize::SPSC::Producer(c, globalTokenBuffer, correctProducerCoordinationBuffer, globalProducerCoordinationBuffer, tokenSize, channelCapacity));
}

TEST(ProducerChannel, Push)
{
  // Creating HWloc topology object
  hwloc_topology_t topology;

  // Reserving memory for hwloc
  hwloc_topology_init(&topology);

  // Instantiating HWloc-based host (CPU) memory manager
  HiCR::backend::host::hwloc::L1::MemoryManager m(&topology);

  // Instantiating Pthread-based host (CPU) communication manager
  HiCR::backend::host::pthreads::L1::CommunicationManager c(1);

  // Initializing HWLoc-based host (CPU) topology manager
  HiCR::backend::host::hwloc::L1::TopologyManager tm(&topology);

  // Asking backend to check the available devices
  const auto t = tm.queryTopology();

  // Getting first device found
  auto d = *t.getDevices().begin();

  // Obtaining memory spaces
  auto memSpaces = d->getMemorySpaceList();

  // Channel configuration
  const auto tokenSize       = sizeof(size_t);
  const auto channelCapacity = 16;

  // Allocating correct memory slots
  auto tokenBuffer                = m.allocateLocalMemorySlot(*memSpaces.begin(), HiCR::channel::fixedSize::SPSC::Consumer::getTokenBufferSize(tokenSize, channelCapacity));
  auto producerCoordinationBuffer = m.allocateLocalMemorySlot(*memSpaces.begin(), HiCR::channel::fixedSize::SPSC::Producer::getCoordinationBufferSize());
  auto consumerCoordinationBuffer = m.allocateLocalMemorySlot(*memSpaces.begin(), HiCR::channel::fixedSize::SPSC::Consumer::getCoordinationBufferSize());

  // Initializing coordination buffer (sets to zero the counters)
  HiCR::channel::fixedSize::SPSC::Producer::initializeCoordinationBuffer(producerCoordinationBuffer);
  HiCR::channel::fixedSize::SPSC::Consumer::initializeCoordinationBuffer(consumerCoordinationBuffer);

  // Exchanging local memory slots to become global for them to be used by the remote end

  c.exchangeGlobalMemorySlots(
    CHANNEL_TAG, {{TOKEN_BUFFER_KEY, tokenBuffer}, {PRODUCER_COORDINATION_BUFFER_KEY, producerCoordinationBuffer}, {CONSUMER_COORDINATION_BUFFER_KEY, consumerCoordinationBuffer}});

  // Synchronizing so that all actors have finished registering their global memory slots
  c.fence(CHANNEL_TAG);

  // Obtaining the globally exchanged memory slots
  auto globalTokenBuffer                = c.getGlobalMemorySlot(CHANNEL_TAG, TOKEN_BUFFER_KEY);
  auto globalProducerCoordinationBuffer = c.getGlobalMemorySlot(CHANNEL_TAG, PRODUCER_COORDINATION_BUFFER_KEY);
  auto globalConsumerCoordinationBuffer = c.getGlobalMemorySlot(CHANNEL_TAG, CONSUMER_COORDINATION_BUFFER_KEY);

  // Creating producer channel
  HiCR::channel::fixedSize::SPSC::Producer producer(c, globalTokenBuffer, producerCoordinationBuffer, globalConsumerCoordinationBuffer, tokenSize, channelCapacity);

  // Creating send buffer
  auto sendBufferCapacity = channelCapacity + 1;
  auto sendBufferSize     = sendBufferCapacity * tokenSize;
  auto sendBuffer         = m.allocateLocalMemorySlot(*memSpaces.begin(), sendBufferSize);

  // Attempting to push no tokens (shouldn't fail)
  EXPECT_NO_THROW(producer.push(sendBuffer, 0));

  // Attempting to push more tokens than buffer size (should throw exception)
  EXPECT_THROW(producer.push(sendBuffer, sendBufferCapacity + 1), HiCR::LogicException);

  // Trying to push more tokens than capacity (should fail)
  EXPECT_THROW(producer.push(sendBuffer, sendBufferCapacity), HiCR::RuntimeException);

  // Trying to push one token
  EXPECT_NO_THROW(producer.push(sendBuffer, 1));

  // Trying to push capacity, but after having pushed one, should fail
  EXPECT_THROW(producer.push(sendBuffer, channelCapacity), HiCR::RuntimeException);

  // Trying to push to capacity, should't fail
  EXPECT_NO_THROW(producer.push(sendBuffer, channelCapacity - 1));

  // Channel is full but trying to push zero should't fail
  EXPECT_NO_THROW(producer.push(sendBuffer, 0));

  // Channel is full and trying to push one more should fail
  EXPECT_THROW(producer.push(sendBuffer, 1), HiCR::RuntimeException);
}

TEST(ProducerChannel, PushWait)
{
  // Creating HWloc topology object
  hwloc_topology_t topology;

  // Reserving memory for hwloc
  hwloc_topology_init(&topology);

  // Instantiating HWloc-based host (CPU) memory manager
  HiCR::backend::host::hwloc::L1::MemoryManager m(&topology);

  // Instantiating Pthread-based host (CPU) communication manager
  HiCR::backend::host::pthreads::L1::CommunicationManager c(1);

  // Initializing HWLoc-based host (CPU) topology manager
  HiCR::backend::host::hwloc::L1::TopologyManager tm(&topology);

  // Asking backend to check the available devices
  const auto t = tm.queryTopology();

  // Getting first device found
  auto d = *t.getDevices().begin();

  // Obtaining memory spaces
  auto memSpaces = d->getMemorySpaceList();

  // Channel configuration
  const auto tokenSize       = sizeof(size_t);
  const auto channelCapacity = 2;

  // Allocating correct memory slots
  auto tokenBuffer                = m.allocateLocalMemorySlot(*memSpaces.begin(), HiCR::channel::fixedSize::SPSC::Consumer::getTokenBufferSize(tokenSize, channelCapacity));
  auto producerCoordinationBuffer = m.allocateLocalMemorySlot(*memSpaces.begin(), HiCR::channel::fixedSize::SPSC::Producer::getCoordinationBufferSize());
  auto consumerCoordinationBuffer = m.allocateLocalMemorySlot(*memSpaces.begin(), HiCR::channel::fixedSize::SPSC::Consumer::getCoordinationBufferSize());

  // Initializing coordination buffer (sets to zero the counters)
  HiCR::channel::fixedSize::SPSC::Producer::initializeCoordinationBuffer(producerCoordinationBuffer);
  HiCR::channel::fixedSize::SPSC::Consumer::initializeCoordinationBuffer(consumerCoordinationBuffer);

  // Exchanging local memory slots to become global for them to be used by the remote end
  c.exchangeGlobalMemorySlots(
    CHANNEL_TAG, {{TOKEN_BUFFER_KEY, tokenBuffer}, {PRODUCER_COORDINATION_BUFFER_KEY, producerCoordinationBuffer}, {CONSUMER_COORDINATION_BUFFER_KEY, consumerCoordinationBuffer}});

  // Synchronizing so that all actors have finished registering their global memory slots
  c.fence(CHANNEL_TAG);

  // Obtaining the globally exchanged memory slots
  auto globalTokenBuffer                = c.getGlobalMemorySlot(CHANNEL_TAG, TOKEN_BUFFER_KEY);
  auto globalProducerCoordinationBuffer = c.getGlobalMemorySlot(CHANNEL_TAG, PRODUCER_COORDINATION_BUFFER_KEY);
  auto globalConsumerCoordinationBuffer = c.getGlobalMemorySlot(CHANNEL_TAG, CONSUMER_COORDINATION_BUFFER_KEY);

  // Creating producer and consumer channels
  HiCR::channel::fixedSize::SPSC::Producer producer(c, globalTokenBuffer, producerCoordinationBuffer, globalConsumerCoordinationBuffer, tokenSize, channelCapacity);
  HiCR::channel::fixedSize::SPSC::Consumer consumer(c, globalTokenBuffer, consumerCoordinationBuffer, globalProducerCoordinationBuffer, tokenSize, channelCapacity);

  // Creating send buffer
  auto sendBufferCapacity = channelCapacity + 1;
  auto sendBufferSize     = sendBufferCapacity * tokenSize;
  auto sendBuffer         = m.allocateLocalMemorySlot(*memSpaces.begin(), sendBufferSize);

  // Attempting to push more tokens than buffer size (should throw exception)
  EXPECT_THROW(producer.push(sendBuffer, sendBufferCapacity + 1), HiCR::LogicException);

  // Trying to fill send buffer (shouldn't wait)
  EXPECT_NO_THROW(producer.push(sendBuffer, channelCapacity));

  // Producer function
  auto producerFc = [&producer, &sendBuffer]() {
    // Wait until the channel gets freed up
    while (producer.isFull()) producer.updateDepth();

    // Now do push message
    producer.push(sendBuffer, 1);
  };

  // Running producer thread that tries to push one token and enters suspension
  std::thread producerThread(producerFc);

  // Waiting until consumer gets the message
  while (consumer.isEmpty()) consumer.updateDepth();

  // Popping one element to liberate thread
  consumer.pop();

  // Wait for producer
  producerThread.join();
}
