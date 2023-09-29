/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file producerChannel.cpp
 * @brief Unit tests for the producerChannel class
 * @author S. M. Martin
 * @date 19/9/2023
 */

#include "gtest/gtest.h"
#include <hicr/backends/sequential/sequential.hpp>
#include <hicr/channel/consumerChannel.hpp>
#include <hicr/channel/producerChannel.hpp>

TEST(ProducerChannel, Construction)
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
  EXPECT_THROW(new HiCR::ProducerChannel(&backend, correctDataBuffer, correctCoordinationBuffer, 0, channelCapacity), HiCR::common::LogicException);
  EXPECT_THROW(new HiCR::ProducerChannel(&backend, correctDataBuffer, correctCoordinationBuffer, tokenSize, 0), HiCR::common::LogicException);
  EXPECT_THROW(new HiCR::ProducerChannel(&backend, correctDataBuffer, badCoordinationBuffer, tokenSize, channelCapacity), HiCR::common::LogicException);

  // Creating with correct parametes
  EXPECT_NO_THROW(new HiCR::ProducerChannel(&backend, badDataBuffer, correctCoordinationBuffer, tokenSize, channelCapacity));
  EXPECT_NO_THROW(new HiCR::ProducerChannel(&backend, correctDataBuffer, correctCoordinationBuffer, tokenSize, channelCapacity));
}

TEST(ProducerChannel, Push)
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

  // Creating producer channel
  HiCR::ProducerChannel producer(&backend, tokenBuffer, coordinationBuffer, tokenSize, channelCapacity);

  // Creating send buffer
  auto sendBufferCapacity = channelCapacity + 1;
  auto sendBufferSize = sendBufferCapacity * tokenSize;
  auto sendBuffer = backend.allocateLocalMemorySlot(*memSpaces.begin(), sendBufferSize);

  // Attempting to push no tokens (shouldn't fail)
  EXPECT_NO_THROW(producer.push(sendBuffer, 0));

  // Attempting to push more tokens than buffer size (should throw exception)
  EXPECT_THROW(producer.push(sendBuffer, sendBufferCapacity + 1), HiCR::common::LogicException);

  // Trying to push more tokens than capacity (should fail)
  EXPECT_THROW(producer.push(sendBuffer, sendBufferCapacity), HiCR::common::RuntimeException);

  // Trying to push one token
  EXPECT_NO_THROW(producer.push(sendBuffer, 1));

  // Trying to push capacity, but after having pushed one, should fail
  EXPECT_THROW(producer.push(sendBuffer, channelCapacity), HiCR::common::RuntimeException);

  // Trying to push to capacity, should't fail
  EXPECT_NO_THROW(producer.push(sendBuffer, channelCapacity - 1));

  // Channel is full but trying to push zero should't fail
  EXPECT_NO_THROW(producer.push(sendBuffer, 0));

  // Channel is full and trying to push one more should fail
  EXPECT_THROW(producer.push(sendBuffer, 1), HiCR::common::RuntimeException);
}

TEST(ProducerChannel, PushWait)
{
  // Instantiating backend
  HiCR::backend::sequential::Sequential backend;

  // Asking backend to check the available resources
  backend.queryMemorySpaces();

  // Obtaining memory spaces
  auto memSpaces = backend.getMemorySpaceList();

  // Channel configuration
  const auto tokenSize = sizeof(size_t);
  const auto channelCapacity = 2;

  // Allocating correct memory slots
  auto tokenBuffer = backend.allocateLocalMemorySlot(*memSpaces.begin(), HiCR::ConsumerChannel::getTokenBufferSize(tokenSize, channelCapacity));
  auto coordinationBuffer = backend.allocateLocalMemorySlot(*memSpaces.begin(), HiCR::ProducerChannel::getCoordinationBufferSize());

  // Initializing coordination buffer (sets to zero the counters)
  EXPECT_NO_THROW(HiCR::ProducerChannel::initializeCoordinationBuffer(&backend, coordinationBuffer));

  // Creating producer channel
  HiCR::ProducerChannel producer(&backend, tokenBuffer, coordinationBuffer, tokenSize, channelCapacity);

  // Creating send buffer
  auto sendBufferCapacity = channelCapacity + 1;
  auto sendBufferSize = sendBufferCapacity * tokenSize;
  auto sendBuffer = backend.allocateLocalMemorySlot(*memSpaces.begin(), sendBufferSize);

  // Attempting to push more tokens than buffer size (should throw exception)
  EXPECT_THROW(producer.push(sendBuffer, sendBufferCapacity + 1), HiCR::common::LogicException);

  // Trying to fill send buffer (shouldn't wait)
  EXPECT_NO_THROW(producer.push(sendBuffer, channelCapacity));

  // Producer function
  auto producerFc = [&producer, &sendBuffer]()
  {
    // Wait until the channel gets freed up
    while (producer.queryDepth() == channelCapacity)
      ;

    // Now do push message
    producer.push(sendBuffer, 1);
  };

  // Running producer thread that tries to push one token and enters suspension
  std::thread producerThread(producerFc);

  // Creating consumer channel
  HiCR::ConsumerChannel consumer(&backend, tokenBuffer, coordinationBuffer, tokenSize, channelCapacity);

  // Waiting until consumer gets the message
  while (consumer.queryDepth() == 0);

  // Popping one element to liberate thread
  consumer.pop();

  // Wait for producer
  producerThread.join();
}
