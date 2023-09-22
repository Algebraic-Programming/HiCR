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
  auto tokenBufferSize        = HiCR::Channel::getTokenBufferSize(tokenSize, channelCapacity);
  auto coordinationBufferSize = HiCR::Channel::getCoordinationBufferSize();

  // Allocating bad memory slots
  auto badDataBuffer         = backend.allocateLocalMemorySlot(*memSpaces.begin(), tokenBufferSize - 1);
  auto badCoordinationBuffer = backend.allocateLocalMemorySlot(*memSpaces.begin(), coordinationBufferSize - 1);

  // Allocating correct memory slots
  auto correctDataBuffer = backend.allocateLocalMemorySlot(*memSpaces.begin(), tokenBufferSize);
  auto correctCoordinationBuffer = backend.allocateLocalMemorySlot(*memSpaces.begin(), coordinationBufferSize);

  // Creating with incorrect parameters
  EXPECT_THROW(new HiCR::ConsumerChannel(&backend, correctDataBuffer, correctCoordinationBuffer, 0, channelCapacity), HiCR::common::LogicException);
  EXPECT_THROW(new HiCR::ConsumerChannel(&backend, correctDataBuffer, correctCoordinationBuffer, tokenSize, 0), HiCR::common::LogicException);
  EXPECT_THROW(new HiCR::ConsumerChannel(&backend, badDataBuffer, correctCoordinationBuffer, tokenSize, channelCapacity), HiCR::common::LogicException);
  EXPECT_THROW(new HiCR::ConsumerChannel(&backend, correctDataBuffer, badCoordinationBuffer, tokenSize, channelCapacity), HiCR::common::LogicException);

  // Creating with correct parametes
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
  auto tokenBuffer        = backend.allocateLocalMemorySlot(*memSpaces.begin(), HiCR::Channel::getTokenBufferSize(tokenSize, channelCapacity));
  auto coordinationBuffer = backend.allocateLocalMemorySlot(*memSpaces.begin(), HiCR::Channel::getCoordinationBufferSize());

  // Creating producer and Consumer channels
  HiCR::ProducerChannel producer(&backend, tokenBuffer, coordinationBuffer, tokenSize, channelCapacity);
  HiCR::ConsumerChannel consumer(&backend, tokenBuffer, coordinationBuffer, tokenSize, channelCapacity);

  // Creating send buffer
  auto sendBufferCapacity = channelCapacity + 1;
  auto sendBufferSize = sendBufferCapacity * tokenSize;
  auto sendBuffer = backend.allocateLocalMemorySlot(*memSpaces.begin(), sendBufferSize);

  // Attempting to pop on an empty channel
  bool result = true;
  EXPECT_NO_THROW(result = consumer.pop());
  EXPECT_FALSE(result);

  // Pushing zero tokens and attempting pop again
  result = true;
  producer.push(sendBuffer, 0);
  EXPECT_NO_THROW(result = consumer.pop());
  EXPECT_FALSE(result);

  // Pushing one token and attempting pop again
  result = false;
  producer.push(sendBuffer, 1);
  EXPECT_NO_THROW(result = consumer.pop());
  EXPECT_TRUE(result);


//  // Attempting to push more tokens than buffer size (should throw exception)
//  EXPECT_THROW(producer.push(sendBuffer, sendBufferCapacity + 1), HiCR::common::LogicException);
//
//  // Trying to push more tokens than capacity (should return false)
//  bool result = true;
//  EXPECT_NO_THROW(result = producer.push(sendBuffer, sendBufferCapacity));
//  EXPECT_FALSE(result);
//
//  // Trying to push one token
//  result = false;
//  EXPECT_NO_THROW(result = producer.push(sendBuffer, 1));
//  EXPECT_TRUE(result);
//
//  // Trying to push capacity, but after having pushed one, should fail
//  result = false;
//  EXPECT_NO_THROW(result = producer.push(sendBuffer, channelCapacity));
//  EXPECT_FALSE(result);
//
//  // Trying to push to capacity, should't fail
//  result = false;
//  EXPECT_NO_THROW(result = producer.push(sendBuffer, channelCapacity - 1));
//  EXPECT_TRUE(result);
//
//  // Channel is full but trying to push zero should't fail
//  result = false;
//  EXPECT_NO_THROW(result = producer.push(sendBuffer, 0));
//  EXPECT_TRUE(result);
//
//  // Channel is full and trying to push one more should fail
//  result = false;
//  EXPECT_NO_THROW(result = producer.push(sendBuffer, 1));
//  EXPECT_FALSE(result);
}
