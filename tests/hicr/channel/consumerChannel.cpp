/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file consumerChannel.cpp
 * @brief Unit tests for the consumerChannel class
 * @author S. M. Martin
 * @date 19/9/2023
 */

#include "gtest/gtest.h"
#include <hicr/backends/sequential/sequential.hpp>
#include <hicr/channel/consumerChannel.hpp>
#include <hicr/channel/producerChannel.hpp>

TEST(ConsumerChannel, Construction)
{
  // Instantiating backend
  HiCR::backend::sequential::Sequential backend;

  // Asking backend to check the available resources
  backend.queryMemorySpaces();

  // Obtaining memory spaces
  auto memSpaces = backend.getMemorySpaceList();

  // Channel configuration
  const auto tokenSize = sizeof(size_t);
  const auto channelCapacity = 16;

  // Getting required buffer sizes
  auto tokenBufferSize = HiCR::ConsumerChannel::getTokenBufferSize(tokenSize, channelCapacity);
  auto coordinationBufferSize = HiCR::ProducerChannel::getCoordinationBufferSize();

  // Allocating bad memory slots
  auto badDataBuffer = backend.allocateLocalMemorySlot(*memSpaces.begin(), tokenBufferSize - 1);
  auto badCoordinationBuffer = backend.allocateLocalMemorySlot(*memSpaces.begin(), coordinationBufferSize - 1);

  // Allocating correct memory slots
  auto correctDataBuffer = backend.allocateLocalMemorySlot(*memSpaces.begin(), tokenBufferSize);
  auto correctCoordinationBuffer = backend.allocateLocalMemorySlot(*memSpaces.begin(), coordinationBufferSize);

  // Creating with incorrect parameters
  EXPECT_THROW(new HiCR::ConsumerChannel(&backend, correctDataBuffer, correctCoordinationBuffer, 0, channelCapacity), HiCR::common::LogicException);
  EXPECT_THROW(new HiCR::ConsumerChannel(&backend, correctDataBuffer, correctCoordinationBuffer, tokenSize, 0), HiCR::common::LogicException);
  EXPECT_THROW(new HiCR::ConsumerChannel(&backend, badDataBuffer, correctCoordinationBuffer, tokenSize, channelCapacity), HiCR::common::LogicException);

  // Creating with correct parametes
  EXPECT_NO_THROW(new HiCR::ConsumerChannel(&backend, correctDataBuffer, badCoordinationBuffer, tokenSize, channelCapacity));
  EXPECT_NO_THROW(new HiCR::ConsumerChannel(&backend, correctDataBuffer, correctCoordinationBuffer, tokenSize, channelCapacity));
}

TEST(ConsumerChannel, PeekPop)
{
  // Instantiating backend
  HiCR::backend::sequential::Sequential backend;

  // Asking backend to check the available resources
  backend.queryMemorySpaces();

  // Obtaining memory spaces
  auto memSpaces = backend.getMemorySpaceList();

  // Channel configuration
  const auto tokenSize = sizeof(size_t);
  const auto channelCapacity = 16;

  // Allocating correct memory slots
  auto tokenBuffer = backend.allocateLocalMemorySlot(*memSpaces.begin(), HiCR::ConsumerChannel::getTokenBufferSize(tokenSize, channelCapacity));
  auto coordinationBuffer = backend.allocateLocalMemorySlot(*memSpaces.begin(), HiCR::ProducerChannel::getCoordinationBufferSize());

  // Initializing coordination buffer (sets to zero the counters)
  HiCR::ProducerChannel::initializeCoordinationBuffer(&backend, coordinationBuffer);

  // Creating producer and Consumer channels
  HiCR::ProducerChannel producer(&backend, tokenBuffer, coordinationBuffer, tokenSize, channelCapacity);
  HiCR::ConsumerChannel consumer(&backend, tokenBuffer, coordinationBuffer, tokenSize, channelCapacity);

  // Creating send buffer
  auto sendBufferCapacity = channelCapacity + 1;
  auto sendBufferSize = sendBufferCapacity * tokenSize;
  auto sendBuffer = backend.allocateLocalMemorySlot(*memSpaces.begin(), sendBufferSize);

  // Attempting pop and peek
  EXPECT_NO_THROW(consumer.pop());
  EXPECT_NO_THROW(consumer.peek());

  // Attempting to pop/peek more than capacity
  EXPECT_THROW(consumer.pop(channelCapacity + 1), HiCR::common::LogicException);
  EXPECT_THROW(consumer.peek(channelCapacity + 1), HiCR::common::LogicException);

  // Attempting to pop on an empty channel
  EXPECT_FALSE(consumer.pop());
  EXPECT_TRUE(consumer.peek().empty());

  // Pushing zero tokens and attempting pop again
  producer.push(sendBuffer, 0);
  EXPECT_FALSE(consumer.pop());
  EXPECT_TRUE(consumer.peek().empty());

  // Pushing one token and attempting pop again
  producer.push(sendBuffer, 1);
  EXPECT_FALSE(consumer.peek().empty());
  EXPECT_TRUE(consumer.peek(2).empty());
  EXPECT_TRUE(consumer.pop());
  EXPECT_TRUE(consumer.peek().empty());

  // Attempting to pop again
  EXPECT_FALSE(consumer.pop());
  EXPECT_TRUE(consumer.peek().empty());
}

TEST(ConsumerChannel, PeekWait)
{
  // Instantiating backend
  HiCR::backend::sequential::Sequential backend;

  // Asking backend to check the available resources
  backend.queryMemorySpaces();

  // Obtaining memory spaces
  auto memSpaces = backend.getMemorySpaceList();

  // Channel configuration
  const auto tokenSize = sizeof(size_t);
  const auto channelCapacity = 1;

  // Allocating correct memory slots
  auto tokenBuffer = backend.allocateLocalMemorySlot(*memSpaces.begin(), HiCR::ConsumerChannel::getTokenBufferSize(tokenSize, channelCapacity));
  auto coordinationBuffer = backend.allocateLocalMemorySlot(*memSpaces.begin(), HiCR::ProducerChannel::getCoordinationBufferSize());

  // Initializing coordination buffer (sets to zero the counters)
  HiCR::ProducerChannel::initializeCoordinationBuffer(&backend, coordinationBuffer);

  // Calculating pointer to the value
  auto recvBuffer = (size_t *)backend.getLocalMemorySlotPointer(tokenBuffer);

  // Creating channel
  HiCR::ProducerChannel producer(&backend, tokenBuffer, coordinationBuffer, tokenSize, channelCapacity);
  HiCR::ConsumerChannel consumer(&backend, tokenBuffer, coordinationBuffer, tokenSize, channelCapacity);

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
    while (consumer.queryDepth() < 1)
      ;

    // Now do the peeking
    auto posVector = consumer.peek(1);

    // Raise consumed flag and read actual value
    hasConsumed = true;
    readValue = recvBuffer[posVector[0]];

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
  auto sendBufferSlot = backend.allocateLocalMemorySlot(*memSpaces.begin(), sendBufferSize);
  auto sendBuffer = (size_t *)backend.getLocalMemorySlotPointer(sendBufferSlot);
  *sendBuffer = expectedValue;

  // Pushing message
  EXPECT_FALSE(hasConsumed);
  producer.push(sendBufferSlot, 1);

  // Wait for producer
  consumerThread.join();

  // Check passed value is correct
  EXPECT_TRUE(hasConsumed);
  EXPECT_EQ(readValue, expectedValue);
  EXPECT_FALSE(consumer.pop());
}
