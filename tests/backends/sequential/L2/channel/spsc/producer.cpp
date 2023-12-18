/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file spsc/producer.cpp
 * @brief Unit tests for the SPSC Producer Channel class
 * @author S. M. Martin
 * @date 19/9/2023
 */

#include "gtest/gtest.h"
#include <backends/sequential/L1/memoryManager.hpp>
#include <backends/sequential/L1/deviceManager.hpp>
#include <hicr/L2/channel/spsc/consumer.hpp>
#include <hicr/L2/channel/spsc/producer.hpp>
#include <thread>

TEST(ProducerChannel, Construction)
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
  auto coordinationBufferSize = HiCR::L2::channel::SPSC::Producer::getCoordinationBufferSize();

  // Allocating bad memory slots
  auto badDataBuffer = m.allocateLocalMemorySlot(*memSpaces.begin(), tokenBufferSize - 1);
  auto badCoordinationBuffer = m.allocateLocalMemorySlot(*memSpaces.begin(), coordinationBufferSize - 1);

  // Allocating correct memory slots
  auto correctDataBuffer = m.allocateLocalMemorySlot(*memSpaces.begin(), tokenBufferSize);
  auto correctCoordinationBuffer = m.allocateLocalMemorySlot(*memSpaces.begin(), coordinationBufferSize);

  // Creating with incorrect parameters
  EXPECT_THROW(new HiCR::L2::channel::SPSC::Producer(&m, correctDataBuffer, correctCoordinationBuffer, 0, channelCapacity), HiCR::common::LogicException);
  EXPECT_THROW(new HiCR::L2::channel::SPSC::Producer(&m, correctDataBuffer, correctCoordinationBuffer, tokenSize, 0), HiCR::common::LogicException);
  EXPECT_THROW(new HiCR::L2::channel::SPSC::Producer(&m, correctDataBuffer, badCoordinationBuffer, tokenSize, channelCapacity), HiCR::common::LogicException);
  EXPECT_THROW(new HiCR::L2::channel::SPSC::Producer(&m, badDataBuffer, correctCoordinationBuffer, tokenSize, channelCapacity), HiCR::common::LogicException);

  // Creating with correct parameters
  EXPECT_NO_THROW(new HiCR::L2::channel::SPSC::Producer(&m, correctDataBuffer, correctCoordinationBuffer, tokenSize, channelCapacity));
}

TEST(ProducerChannel, Push)
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
  auto coordinationBuffer = m.allocateLocalMemorySlot(*memSpaces.begin(), HiCR::L2::channel::SPSC::Producer::getCoordinationBufferSize());

  // Initializing coordination buffer (sets to zero the counters)
  HiCR::L2::channel::SPSC::Producer::initializeCoordinationBuffer(coordinationBuffer);

  // Creating producer channel
  HiCR::L2::channel::SPSC::Producer producer(&m, tokenBuffer, coordinationBuffer, tokenSize, channelCapacity);

  // Creating send buffer
  auto sendBufferCapacity = channelCapacity + 1;
  auto sendBufferSize = sendBufferCapacity * tokenSize;
  auto sendBuffer = m.allocateLocalMemorySlot(*memSpaces.begin(), sendBufferSize);

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
  const auto channelCapacity = 2;

  // Allocating correct memory slots
  auto tokenBuffer = m.allocateLocalMemorySlot(*memSpaces.begin(), HiCR::L2::channel::SPSC::Consumer::getTokenBufferSize(tokenSize, channelCapacity));
  auto producerCoordinationBuffer = m.allocateLocalMemorySlot(*memSpaces.begin(), HiCR::L2::channel::SPSC::Producer::getCoordinationBufferSize());
  auto consumerCoordinationBuffer = m.allocateLocalMemorySlot(*memSpaces.begin(), HiCR::L2::channel::SPSC::Consumer::getCoordinationBufferSize());

  // Initializing coordination buffer (sets to zero the counters)
  EXPECT_NO_THROW(HiCR::L2::channel::SPSC::Producer::initializeCoordinationBuffer(producerCoordinationBuffer));
  EXPECT_NO_THROW(HiCR::L2::channel::SPSC::Consumer::initializeCoordinationBuffer(consumerCoordinationBuffer));

  // Creating producer channel
  HiCR::L2::channel::SPSC::Producer producer(&m, tokenBuffer, producerCoordinationBuffer, tokenSize, channelCapacity);

  // Creating send buffer
  auto sendBufferCapacity = channelCapacity + 1;
  auto sendBufferSize = sendBufferCapacity * tokenSize;
  auto sendBuffer = m.allocateLocalMemorySlot(*memSpaces.begin(), sendBufferSize);

  // Attempting to push more tokens than buffer size (should throw exception)
  EXPECT_THROW(producer.push(sendBuffer, sendBufferCapacity + 1), HiCR::common::LogicException);

  // Trying to fill send buffer (shouldn't wait)
  EXPECT_NO_THROW(producer.push(sendBuffer, channelCapacity));

  // Producer function
  auto producerFc = [&producer, &sendBuffer]()
  {
    // Wait until the channel gets freed up
    while (producer.getDepth() == channelCapacity) producer.updateDepth();

    // Now do push message
    producer.push(sendBuffer, 1);
  };

  // Running producer thread that tries to push one token and enters suspension
  std::thread producerThread(producerFc);

  // Creating consumer channel
  HiCR::L2::channel::SPSC::Consumer consumer(&m, tokenBuffer, consumerCoordinationBuffer, producerCoordinationBuffer, tokenSize, channelCapacity);

  // Waiting until consumer gets the message
  while (consumer.getDepth() == 0) consumer.updateDepth();

  // Popping one element to liberate thread
  consumer.pop();

  // Wait for producer
  producerThread.join();
}
