/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file spsc/consumer.cpp
 * @brief Unit tests for the SPSC Consumer Channel class
 * @author S. M. Martin
 * @date 19/9/2023
 */

#include <thread>
#include "gtest/gtest.h"
#include <hicr/exceptions.hpp>
#include <frontends/channel/fixedSize/spsc/consumer.hpp>
#include <frontends/channel/fixedSize/spsc/producer.hpp>
#include <backends/host/hwloc/L1/memoryManager.hpp>
#include <backends/host/pthreads/L1/communicationManager.hpp>
#include <backends/host/hwloc/L1/topologyManager.hpp>

#define CHANNEL_TAG 0
#define TOKEN_BUFFER_KEY 0
#define PRODUCER_COORDINATION_BUFFER_KEY 1
#define BAD_CONSUMER_COORDINATION_BUFFER_KEY 3
#define BAD_TOKEN_BUFFER_KEY 4

TEST(ConsumerChannel, Construction)
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
  auto tokenBufferSize                = HiCR::channel::fixedSize::SPSC::Consumer::getTokenBufferSize(tokenSize, channelCapacity);
  auto producerCoordinationBufferSize = HiCR::channel::fixedSize::SPSC::Producer::getCoordinationBufferSize();
  auto consumerCoordinationBufferSize = HiCR::channel::fixedSize::SPSC::Producer::getCoordinationBufferSize();

  // Allocating bad memory slots
  auto badTokenBuffer = m.allocateLocalMemorySlot(*memSpaces.begin(), tokenBufferSize - 1);

  // Allocating correct memory slots
  auto tokenBuffer                   = m.allocateLocalMemorySlot(*memSpaces.begin(), tokenBufferSize);
  auto consumerCoordinationBuffer    = m.allocateLocalMemorySlot(*memSpaces.begin(), consumerCoordinationBufferSize);
  auto badConsumerCoordinationBuffer = m.allocateLocalMemorySlot(*memSpaces.begin(), consumerCoordinationBufferSize - 1);
  auto producerCoordinationBuffer    = m.allocateLocalMemorySlot(*memSpaces.begin(), producerCoordinationBufferSize);

  // Exchanging local memory slots to become global for them to be used by the remote end
  c.exchangeGlobalMemorySlots(CHANNEL_TAG,
                              {{TOKEN_BUFFER_KEY, tokenBuffer}, {PRODUCER_COORDINATION_BUFFER_KEY, producerCoordinationBuffer}, {BAD_TOKEN_BUFFER_KEY, badTokenBuffer}});

  // Synchronizing so that all actors have finished registering their global memory slots
  c.fence(CHANNEL_TAG);

  // Obtaining the globally exchanged memory slots
  auto globalTokenBuffer                = c.getGlobalMemorySlot(CHANNEL_TAG, TOKEN_BUFFER_KEY);
  auto globalProducerCoordinationBuffer = c.getGlobalMemorySlot(CHANNEL_TAG, PRODUCER_COORDINATION_BUFFER_KEY);
  auto badGlobalTokenBuffer             = c.getGlobalMemorySlot(CHANNEL_TAG, BAD_TOKEN_BUFFER_KEY);

  // Creating with incorrect parameters
  EXPECT_THROW(new HiCR::channel::fixedSize::SPSC::Consumer(c, globalTokenBuffer, badConsumerCoordinationBuffer, globalProducerCoordinationBuffer, tokenSize, channelCapacity),
               HiCR::LogicException);
  EXPECT_THROW(new HiCR::channel::fixedSize::SPSC::Consumer(c, badGlobalTokenBuffer, consumerCoordinationBuffer, globalProducerCoordinationBuffer, tokenSize, channelCapacity),
               HiCR::LogicException);

  // Creating with correct parameters
  EXPECT_NO_THROW(new HiCR::channel::fixedSize::SPSC::Consumer(c, globalTokenBuffer, consumerCoordinationBuffer, globalProducerCoordinationBuffer, tokenSize, channelCapacity));
}

TEST(ConsumerChannel, PeekPop)
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
  c.exchangeGlobalMemorySlots(CHANNEL_TAG, {{TOKEN_BUFFER_KEY, tokenBuffer}, {PRODUCER_COORDINATION_BUFFER_KEY, producerCoordinationBuffer}});

  // Synchronizing so that all actors have finished registering their global memory slots
  c.fence(CHANNEL_TAG);

  // Obtaining the globally exchanged memory slots
  auto globalTokenBuffer                = c.getGlobalMemorySlot(CHANNEL_TAG, TOKEN_BUFFER_KEY);
  auto globalProducerCoordinationBuffer = c.getGlobalMemorySlot(CHANNEL_TAG, PRODUCER_COORDINATION_BUFFER_KEY);

  // Creating producer and Consumer channels
  HiCR::channel::fixedSize::SPSC::Producer producer(c, globalTokenBuffer, producerCoordinationBuffer, globalProducerCoordinationBuffer, tokenSize, channelCapacity);
  HiCR::channel::fixedSize::SPSC::Consumer consumer(c, globalTokenBuffer, consumerCoordinationBuffer, globalProducerCoordinationBuffer, tokenSize, channelCapacity);

  // Creating send buffer
  auto sendBufferCapacity = channelCapacity + 1;
  auto sendBufferSize     = sendBufferCapacity * tokenSize;
  auto sendBuffer         = m.allocateLocalMemorySlot(*memSpaces.begin(), sendBufferSize);

  // Attempting pop and peek
  EXPECT_THROW(consumer.pop(), HiCR::RuntimeException);
  EXPECT_THROW(consumer.peek(), HiCR::RuntimeException);

  // Attempting to pop/peek more than capacity
  EXPECT_THROW(consumer.pop(channelCapacity + 1), HiCR::LogicException);
  EXPECT_THROW(consumer.peek(channelCapacity + 1), HiCR::LogicException);

  // Attempting to pop on an empty channel
  EXPECT_THROW(consumer.pop(), HiCR::RuntimeException);
  EXPECT_THROW(consumer.peek(), HiCR::RuntimeException);

  // Pushing zero tokens and attempting pop again
  producer.push(sendBuffer, 0);
  EXPECT_THROW(consumer.pop(), HiCR::RuntimeException);
  EXPECT_THROW(consumer.peek(), HiCR::RuntimeException);

  // Pushing one token and attempting pop again
  producer.push(sendBuffer, 1);
  EXPECT_NO_THROW(consumer.peek());
  EXPECT_THROW(consumer.peek(2), HiCR::RuntimeException);
  EXPECT_NO_THROW(consumer.pop());
  EXPECT_THROW(consumer.peek(), HiCR::RuntimeException);

  // Attempting to pop again
  EXPECT_THROW(consumer.pop(), HiCR::RuntimeException);
  EXPECT_THROW(consumer.peek(), HiCR::RuntimeException);
}

TEST(ConsumerChannel, PeekWait)
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
  const auto channelCapacity = 1;

  // Allocating correct memory slots
  auto tokenBuffer                = m.allocateLocalMemorySlot(*memSpaces.begin(), HiCR::channel::fixedSize::SPSC::Consumer::getTokenBufferSize(tokenSize, channelCapacity));
  auto producerCoordinationBuffer = m.allocateLocalMemorySlot(*memSpaces.begin(), HiCR::channel::fixedSize::SPSC::Producer::getCoordinationBufferSize());
  auto consumerCoordinationBuffer = m.allocateLocalMemorySlot(*memSpaces.begin(), HiCR::channel::fixedSize::SPSC::Consumer::getCoordinationBufferSize());

  // Initializing coordination buffer (sets to zero the counters)
  HiCR::channel::fixedSize::SPSC::Producer::initializeCoordinationBuffer(producerCoordinationBuffer);
  HiCR::channel::fixedSize::SPSC::Consumer::initializeCoordinationBuffer(consumerCoordinationBuffer);

  // Calculating pointer to the value
  auto recvBuffer = (size_t *)tokenBuffer->getPointer();

  // Exchanging local memory slots to become global for them to be used by the remote end
  c.exchangeGlobalMemorySlots(CHANNEL_TAG, {{TOKEN_BUFFER_KEY, tokenBuffer}, {PRODUCER_COORDINATION_BUFFER_KEY, producerCoordinationBuffer}});

  // Synchronizing so that all actors have finished registering their global memory slots
  c.fence(CHANNEL_TAG);

  // Obtaining the globally exchanged memory slots
  auto globalTokenBuffer                = c.getGlobalMemorySlot(CHANNEL_TAG, TOKEN_BUFFER_KEY);
  auto globalProducerCoordinationBuffer = c.getGlobalMemorySlot(CHANNEL_TAG, PRODUCER_COORDINATION_BUFFER_KEY);

  // Creating channel
  HiCR::channel::fixedSize::SPSC::Producer producer(c, globalTokenBuffer, producerCoordinationBuffer, globalProducerCoordinationBuffer, tokenSize, channelCapacity);
  HiCR::channel::fixedSize::SPSC::Consumer consumer(c, globalTokenBuffer, consumerCoordinationBuffer, globalProducerCoordinationBuffer, tokenSize, channelCapacity);

  // Attempting to push more tokens than channel size (should throw exception)
  EXPECT_THROW(consumer.peek(channelCapacity + 1), HiCR::LogicException);

  // Producer function
  const size_t expectedValue = 42;
  bool         hasStarted    = false;
  bool         hasConsumed   = false;
  size_t       readValue     = 0;
  auto         consumerFc    = [&consumer, &hasStarted, &hasConsumed, &readValue, &recvBuffer]() {
    hasStarted = true;

    // Wait until the producer pushes a message
    while (consumer.getDepth() < 1) consumer.updateDepth();

    // Raise consumed flag and read actual value
    hasConsumed = true;
    readValue   = recvBuffer[consumer.peek(0)];

    // Pop message
    consumer.pop();
  };

  // Running producer thread that tries to pushWait one token and enters suspension
  std::thread consumerThread(consumerFc);

  // Waiting a bit to make sure the consumer thread has started
  while (hasStarted == false)
    ;

  // The consumer flag should still be false
  sched_yield();
  usleep(50000);
  sched_yield();

  // Creating send buffer
  auto sendBufferCapacity = channelCapacity + 1;
  auto sendBufferSize     = sendBufferCapacity * tokenSize;
  auto sendBufferSlot     = m.allocateLocalMemorySlot(*memSpaces.begin(), sendBufferSize);
  auto sendBuffer         = (size_t *)sendBufferSlot->getPointer();
  *sendBuffer             = expectedValue;

  // Pushing message
  EXPECT_FALSE(hasConsumed);
  producer.push(sendBufferSlot, 1);

  // Wait for producer
  consumerThread.join();

  // Check passed value is correct
  EXPECT_TRUE(hasConsumed);
  EXPECT_EQ(readValue, expectedValue);
  EXPECT_THROW(consumer.pop(), HiCR::RuntimeException);
}
