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

#define CHANNEL_TAG 0
#define TOKEN_BUFFER_KEY 0
#define PRODUCER_COORDINATION_BUFFER_KEY 1

TEST(ProducerChannel, Construction)
{
  // Instantiating backend
  HiCR::backend::sequential::L1::MemoryManager m(1);

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
  auto badCoordinationBuffer = m.allocateLocalMemorySlot(*memSpaces.begin(), coordinationBufferSize - 1);

  // Allocating correct memory slots
  auto correctTokenBuffer = m.allocateLocalMemorySlot(*memSpaces.begin(), tokenBufferSize);
  auto correctCoordinationBuffer = m.allocateLocalMemorySlot(*memSpaces.begin(), coordinationBufferSize);

  // Exchanging local memory slots to become global for them to be used by the remote end
  m.exchangeGlobalMemorySlots(CHANNEL_TAG, {
                                             {PRODUCER_COORDINATION_BUFFER_KEY, correctCoordinationBuffer},
                                             {TOKEN_BUFFER_KEY, correctTokenBuffer},
                                             });

  // Synchronizing so that all actors have finished registering their global memory slots
  m.fence(CHANNEL_TAG);

  // Obtaining the globally exchanged memory slots
  auto globalTokenBuffer = m.getGlobalMemorySlot(CHANNEL_TAG, TOKEN_BUFFER_KEY);
  auto globalProducerCoordinationBuffer = m.getGlobalMemorySlot(CHANNEL_TAG, PRODUCER_COORDINATION_BUFFER_KEY);

  // Creating with incorrect parameters
  EXPECT_THROW(new HiCR::L2::channel::SPSC::Producer(&m, globalTokenBuffer, badCoordinationBuffer, globalProducerCoordinationBuffer, tokenSize, channelCapacity), HiCR::common::LogicException);

  // Creating with correct parameters
  EXPECT_NO_THROW(new HiCR::L2::channel::SPSC::Producer(&m, globalTokenBuffer, correctCoordinationBuffer, globalProducerCoordinationBuffer, tokenSize, channelCapacity));
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

 // Exchanging local memory slots to become global for them to be used by the remote end
  m.exchangeGlobalMemorySlots(CHANNEL_TAG, {
                                             {PRODUCER_COORDINATION_BUFFER_KEY, coordinationBuffer},
                                             {TOKEN_BUFFER_KEY, tokenBuffer},
                                             });

  // Synchronizing so that all actors have finished registering their global memory slots
  m.fence(CHANNEL_TAG);

  // Obtaining the globally exchanged memory slots
  auto globalTokenBuffer = m.getGlobalMemorySlot(CHANNEL_TAG, TOKEN_BUFFER_KEY);
  auto globalProducerCoordinationBuffer = m.getGlobalMemorySlot(CHANNEL_TAG, PRODUCER_COORDINATION_BUFFER_KEY);

  // Creating producer channel
  HiCR::L2::channel::SPSC::Producer producer(&m, globalTokenBuffer, coordinationBuffer, globalProducerCoordinationBuffer, tokenSize, channelCapacity);

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

 // Exchanging local memory slots to become global for them to be used by the remote end
  m.exchangeGlobalMemorySlots(CHANNEL_TAG, {
                                             {PRODUCER_COORDINATION_BUFFER_KEY, producerCoordinationBuffer},
                                             {TOKEN_BUFFER_KEY, tokenBuffer},
                                             });

  // Synchronizing so that all actors have finished registering their global memory slots
  m.fence(CHANNEL_TAG);

  // Obtaining the globally exchanged memory slots
  auto globalTokenBuffer = m.getGlobalMemorySlot(CHANNEL_TAG, TOKEN_BUFFER_KEY);
  auto globalProducerCoordinationBuffer = m.getGlobalMemorySlot(CHANNEL_TAG, PRODUCER_COORDINATION_BUFFER_KEY);

  // Initializing coordination buffer (sets to zero the counters)
  EXPECT_NO_THROW(HiCR::L2::channel::SPSC::Producer::initializeCoordinationBuffer(producerCoordinationBuffer));
  EXPECT_NO_THROW(HiCR::L2::channel::SPSC::Consumer::initializeCoordinationBuffer(consumerCoordinationBuffer));

  // Creating producer channel
  HiCR::L2::channel::SPSC::Producer producer(&m, globalTokenBuffer, producerCoordinationBuffer, globalProducerCoordinationBuffer, tokenSize, channelCapacity);

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
  HiCR::L2::channel::SPSC::Consumer consumer(&m, globalTokenBuffer, consumerCoordinationBuffer, globalProducerCoordinationBuffer, tokenSize, channelCapacity);

  // Waiting until consumer gets the message
  while (consumer.getDepth() == 0) consumer.updateDepth();

  // Popping one element to liberate thread
  consumer.pop();

  // Wait for producer
  producerThread.join();
}
