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

#include "gtest/gtest.h"
#include <backends/sequential/L1/deviceManager.hpp>
#include <backends/sequential/L1/memoryManager.hpp>
#include <hicr/L2/channel/spsc/consumer.hpp>
#include <hicr/L2/channel/spsc/producer.hpp>
#include <hicr/common/exceptions.hpp>
#include <thread>

TEST(ConsumerChannel, Construction)
{
  // Instantiating backend
  HiCR::backend::sequential::L1::MemoryManager m;

// Initializing Sequential backend's device manager
  HiCR::backend::sequential::L1::DeviceManager dm;

  // Asking backend to check the available devices
  dm.queryDevices();

  // Getting first device found
  auto d = *dm.getDevices().begin();

  // Obtaining memory spaces
  auto memSpaces = d->getMemorySpaceList();

  // Channel configuration
  const auto tokenSize = sizeof(size_t);
  const auto channelCapacity = 16;

  // Getting required buffer sizes
  auto tokenBufferSize = HiCR::L2::channel::SPSC::Consumer::getTokenBufferSize(tokenSize, channelCapacity);
  auto producerCoordinationBufferSize = HiCR::L2::channel::SPSC::Producer::getCoordinationBufferSize();
  auto consumerCoordinationBufferSize = HiCR::L2::channel::SPSC::Producer::getCoordinationBufferSize();

  // Allocating bad memory slots
  auto badDataBuffer = m.allocateLocalMemorySlot(*memSpaces.begin(), tokenBufferSize - 1);
  auto badCoordinationBuffer = m.allocateLocalMemorySlot(*memSpaces.begin(), consumerCoordinationBufferSize - 1);

  // Allocating correct memory slots
  auto correctDataBuffer = m.allocateLocalMemorySlot(*memSpaces.begin(), tokenBufferSize);
  auto correctCoordinationBuffer = m.allocateLocalMemorySlot(*memSpaces.begin(), consumerCoordinationBufferSize);
  auto producerCoordinationBuffer = m.allocateLocalMemorySlot(*memSpaces.begin(), producerCoordinationBufferSize);

  // Creating with incorrect parameters
  EXPECT_THROW(new HiCR::L2::channel::SPSC::Consumer(&m, correctDataBuffer, correctCoordinationBuffer, producerCoordinationBuffer, 0, channelCapacity), HiCR::common::LogicException);
  EXPECT_THROW(new HiCR::L2::channel::SPSC::Consumer(&m, correctDataBuffer, correctCoordinationBuffer, producerCoordinationBuffer, tokenSize, 0), HiCR::common::LogicException);
  EXPECT_THROW(new HiCR::L2::channel::SPSC::Consumer(&m, badDataBuffer, correctCoordinationBuffer, producerCoordinationBuffer, tokenSize, channelCapacity), HiCR::common::LogicException);
  EXPECT_THROW(new HiCR::L2::channel::SPSC::Consumer(&m, correctDataBuffer, badCoordinationBuffer, producerCoordinationBuffer, tokenSize, channelCapacity), HiCR::common::LogicException);

  // Creating with correct parameters
  EXPECT_NO_THROW(new HiCR::L2::channel::SPSC::Consumer(&m, correctDataBuffer, correctCoordinationBuffer, producerCoordinationBuffer, tokenSize, channelCapacity));
}

TEST(ConsumerChannel, PeekPop)
{
  // Instantiating backend
  HiCR::backend::sequential::L1::MemoryManager m;

// Initializing Sequential backend's device manager
  HiCR::backend::sequential::L1::DeviceManager dm;

  // Asking backend to check the available devices
  dm.queryDevices();

  // Getting first device found
  auto d = *dm.getDevices().begin();

  // Obtaining memory spaces
  auto memSpaces = d->getMemorySpaceList();

  // Channel configuration
  const auto tokenSize = sizeof(size_t);
  const auto channelCapacity = 16;

  // Allocating correct memory slots
  auto tokenBuffer = m.allocateLocalMemorySlot(*memSpaces.begin(), HiCR::L2::channel::SPSC::Consumer::getTokenBufferSize(tokenSize, channelCapacity));
  auto producerCoordinationBuffer = m.allocateLocalMemorySlot(*memSpaces.begin(), HiCR::L2::channel::SPSC::Producer::getCoordinationBufferSize());
  auto consumerCoordinationBuffer = m.allocateLocalMemorySlot(*memSpaces.begin(), HiCR::L2::channel::SPSC::Consumer::getCoordinationBufferSize());

  // Initializing coordination buffer (sets to zero the counters)
  HiCR::L2::channel::SPSC::Producer::initializeCoordinationBuffer(producerCoordinationBuffer);
  HiCR::L2::channel::SPSC::Consumer::initializeCoordinationBuffer(consumerCoordinationBuffer);

  // Creating producer and Consumer channels
  HiCR::L2::channel::SPSC::Producer producer(&m, tokenBuffer, producerCoordinationBuffer, tokenSize, channelCapacity);
  HiCR::L2::channel::SPSC::Consumer consumer(&m, tokenBuffer, consumerCoordinationBuffer, producerCoordinationBuffer, tokenSize, channelCapacity);

  // Creating send buffer
  auto sendBufferCapacity = channelCapacity + 1;
  auto sendBufferSize = sendBufferCapacity * tokenSize;
  auto sendBuffer = m.allocateLocalMemorySlot(*memSpaces.begin(), sendBufferSize);

  // Attempting pop and peek
  EXPECT_THROW(consumer.pop(), HiCR::common::RuntimeException);
  EXPECT_THROW(consumer.peek(), HiCR::common::RuntimeException);

  // Attempting to pop/peek more than capacity
  EXPECT_THROW(consumer.pop(channelCapacity + 1), HiCR::common::LogicException);
  EXPECT_THROW(consumer.peek(channelCapacity + 1), HiCR::common::LogicException);

  // Attempting to pop on an empty channel
  EXPECT_THROW(consumer.pop(), HiCR::common::RuntimeException);
  EXPECT_THROW(consumer.peek(), HiCR::common::RuntimeException);

  // Pushing zero tokens and attempting pop again
  producer.push(sendBuffer, 0);
  EXPECT_THROW(consumer.pop(), HiCR::common::RuntimeException);
  EXPECT_THROW(consumer.peek(), HiCR::common::RuntimeException);

  // Pushing one token and attempting pop again
  producer.push(sendBuffer, 1);
  EXPECT_NO_THROW(consumer.peek());
  EXPECT_THROW(consumer.peek(2), HiCR::common::RuntimeException);
  EXPECT_NO_THROW(consumer.pop());
  EXPECT_THROW(consumer.peek(), HiCR::common::RuntimeException);

  // Attempting to pop again
  EXPECT_THROW(consumer.pop(), HiCR::common::RuntimeException);
  EXPECT_THROW(consumer.peek(), HiCR::common::RuntimeException);
}

TEST(ConsumerChannel, PeekWait)
{
  // Instantiating backend
  HiCR::backend::sequential::L1::MemoryManager m;

// Initializing Sequential backend's device manager
  HiCR::backend::sequential::L1::DeviceManager dm;

  // Asking backend to check the available devices
  dm.queryDevices();

  // Getting first device found
  auto d = *dm.getDevices().begin();

  // Obtaining memory spaces
  auto memSpaces = d->getMemorySpaceList();

  // Channel configuration
  const auto tokenSize = sizeof(size_t);
  const auto channelCapacity = 1;

  // Allocating correct memory slots
  auto tokenBuffer = m.allocateLocalMemorySlot(*memSpaces.begin(), HiCR::L2::channel::SPSC::Consumer::getTokenBufferSize(tokenSize, channelCapacity));
  auto producerCoordinationBuffer = m.allocateLocalMemorySlot(*memSpaces.begin(), HiCR::L2::channel::SPSC::Producer::getCoordinationBufferSize());
  auto consumerCoordinationBuffer = m.allocateLocalMemorySlot(*memSpaces.begin(), HiCR::L2::channel::SPSC::Consumer::getCoordinationBufferSize());

  // Initializing coordination buffer (sets to zero the counters)
  HiCR::L2::channel::SPSC::Producer::initializeCoordinationBuffer(producerCoordinationBuffer);
  HiCR::L2::channel::SPSC::Consumer::initializeCoordinationBuffer(consumerCoordinationBuffer);

  // Calculating pointer to the value
  auto recvBuffer = (size_t *)tokenBuffer->getPointer();

  // Creating channel
  HiCR::L2::channel::SPSC::Producer producer(&m, tokenBuffer, producerCoordinationBuffer, tokenSize, channelCapacity);
  HiCR::L2::channel::SPSC::Consumer consumer(&m, tokenBuffer, consumerCoordinationBuffer, producerCoordinationBuffer, tokenSize, channelCapacity);

  // Attempting to push more tokens than channel size (should throw exception)
  EXPECT_THROW(consumer.peek(channelCapacity + 1), HiCR::common::LogicException);

  // Producer function
  const size_t expectedValue = 42;
  bool hasStarted = false;
  bool hasConsumed = false;
  size_t readValue = 0;
  auto consumerFc = [&consumer, &hasStarted, &hasConsumed, &readValue, &recvBuffer]()
  {
    hasStarted = true;

    // Wait until the producer pushes a message
    while (consumer.getDepth() < 1) consumer.updateDepth();

    // Raise consumed flag and read actual value
    hasConsumed = true;
    readValue = recvBuffer[consumer.peek(0)];

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
  auto sendBufferSize = sendBufferCapacity * tokenSize;
  auto sendBufferSlot = m.allocateLocalMemorySlot(*memSpaces.begin(), sendBufferSize);
  auto sendBuffer = (size_t *)sendBufferSlot->getPointer();
  *sendBuffer = expectedValue;

  // Pushing message
  EXPECT_FALSE(hasConsumed);
  producer.push(sendBufferSlot, 1);

  // Wait for producer
  consumerThread.join();

  // Check passed value is correct
  EXPECT_TRUE(hasConsumed);
  EXPECT_EQ(readValue, expectedValue);
  EXPECT_THROW(consumer.pop(), HiCR::common::RuntimeException);
}
