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
#include <hicr/channel/producerChannel.hpp>
#include <hicr/channel/consumerChannel.hpp>

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
 auto tokenBufferSize = HiCR::Channel::getTokenBufferSize(tokenSize, channelCapacity);
 auto coordinationBufferSize = HiCR::Channel::getCoordinationBufferSize();

 // Allocating bad memory slots
 auto badDataBuffer         = backend.allocateMemorySlot(*memSpaces.begin(), tokenBufferSize - 1);
 auto badCoordinationBuffer = backend.allocateMemorySlot(*memSpaces.begin(), coordinationBufferSize - 1);

 // Allocating correct memory slots
 auto correctDataBuffer         = backend.allocateMemorySlot(*memSpaces.begin(), tokenBufferSize);
 auto correctCoordinationBuffer = backend.allocateMemorySlot(*memSpaces.begin(), coordinationBufferSize);

 // Creating with incorrect parameters
 EXPECT_THROW(new HiCR::ProducerChannel(&backend, correctDataBuffer, correctCoordinationBuffer, 0, channelCapacity), HiCR::common::LogicException);
 EXPECT_THROW(new HiCR::ProducerChannel(&backend, correctDataBuffer, correctCoordinationBuffer, tokenSize, 0), HiCR::common::LogicException);
 EXPECT_THROW(new HiCR::ProducerChannel(&backend, badDataBuffer, correctCoordinationBuffer, tokenSize, channelCapacity), HiCR::common::LogicException);
 EXPECT_THROW(new HiCR::ProducerChannel(&backend, correctDataBuffer, badCoordinationBuffer, tokenSize, channelCapacity), HiCR::common::LogicException);

 // Creating with correct parametes
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
 auto dataBuffer         = backend.allocateMemorySlot(*memSpaces.begin(), channelCapacity * tokenSize);
 auto coordinationBuffer = backend.allocateMemorySlot(*memSpaces.begin(), HiCR::Channel::getCoordinationBufferSize());

 // Creating producer channel
 HiCR::ProducerChannel producer(&backend, dataBuffer, coordinationBuffer, tokenSize, channelCapacity);

 // Creating send buffer
 auto sendBufferCapacity = channelCapacity + 1;
 auto sendBufferSize = sendBufferCapacity * tokenSize;
 auto sendBuffer  = backend.allocateMemorySlot(*memSpaces.begin(), sendBufferSize);

 // Attempting to push no tokens (shouldn't fail)
 EXPECT_NO_THROW(producer.push(sendBuffer, 0));

 // Attempting to push more tokens than buffer size (should throw exception)
 EXPECT_THROW(producer.push(sendBuffer, sendBufferCapacity + 1), HiCR::common::LogicException);

 // Trying to push more tokens than capacity (should return false)
 bool result = true;
 EXPECT_NO_THROW(result = producer.push(sendBuffer, sendBufferCapacity));
 EXPECT_FALSE(result);

 // Trying to push one token
 result = false;
 EXPECT_NO_THROW(result = producer.push(sendBuffer, 1));
 EXPECT_TRUE(result);

 // Trying to push capacity, but after having pushed one, should fail
 result = false;
 EXPECT_NO_THROW(result = producer.push(sendBuffer, channelCapacity));
 EXPECT_FALSE(result);

 // Trying to push to capacity, should't fail
 result = false;
 EXPECT_NO_THROW(result = producer.push(sendBuffer, channelCapacity - 1));
 EXPECT_TRUE(result);

 // Channel is full but trying to push zero should't fail
 result = false;
 EXPECT_NO_THROW(result = producer.push(sendBuffer, 0));
 EXPECT_TRUE(result);

 // Channel is full and trying to push one more should fail
 result = false;
 EXPECT_NO_THROW(result = producer.push(sendBuffer, 1));
 EXPECT_FALSE(result);
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
  auto dataBuffer         = backend.allocateMemorySlot(*memSpaces.begin(), channelCapacity * tokenSize);
  auto coordinationBuffer = backend.allocateMemorySlot(*memSpaces.begin(), HiCR::Channel::getCoordinationBufferSize());

  // Creating producer channel
  HiCR::ProducerChannel producer(&backend, dataBuffer, coordinationBuffer, tokenSize, channelCapacity);

  // Creating send buffer
  auto sendBufferCapacity = channelCapacity + 1;
  auto sendBufferSize = sendBufferCapacity * tokenSize;
  auto sendBuffer  = backend.allocateMemorySlot(*memSpaces.begin(), sendBufferSize);

  // Attempting to push more tokens than buffer size (should throw exception)
  EXPECT_THROW(producer.pushWait(sendBuffer, sendBufferCapacity + 1), HiCR::common::LogicException);

  // Trying to fill send buffer (shouldn't wait)
  EXPECT_NO_THROW(producer.pushWait(sendBuffer, channelCapacity));

  // To actually test for waiting, one needs to use threads
  std::thread* producerThread;

  // Producer function
  auto producerFc = [&producer, &sendBuffer](){ producer.pushWait(sendBuffer, 1); };

  // Running producer thread that tries to pushWait one token and enters suspension
  producerThread = new std::thread(producerFc);

  // Creating consumer channel
  HiCR::ConsumerChannel consumer(&backend, dataBuffer, coordinationBuffer, tokenSize, channelCapacity);

  // Popping one element to liberate thread
  while (consumer.pop() == false);

  // Wait for producer
  producerThread->join();

  // Freeing up memory
  delete producerThread;

}

