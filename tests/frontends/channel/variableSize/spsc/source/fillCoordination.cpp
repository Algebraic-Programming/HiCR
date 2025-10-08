#include <fstream>
#include <mpi.h>
#include <nlohmann_json/json.hpp>

#include <hicr/backends/mpi/communicationManager.hpp>
#include <hicr/backends/mpi/memoryManager.hpp>
#include <hicr/backends/mpi/instanceManager.hpp>
#include <hicr/backends/hwloc/topologyManager.hpp>

#include "../include/channelFixture.hpp"

void producerFc(ChannelFixture &fixture)
{
  // Create producer and pick managers from the fixture
  fixture.producer = fixture.createProducer(
    *fixture.memoryManager, *fixture.memoryManager, *fixture.communicationManager, *fixture.communicationManager, fixture.memorySpace, fixture.memorySpace, CHANNEL_CAPACITY - 1);

  auto &producer                         = *fixture.producer;
  auto &payloadMemoryManager             = *fixture.memoryManager;
  auto &coordinationCommunicationManager = *fixture.communicationManager;
  auto &payloadCommunicationManager      = *fixture.communicationManager;
  auto  payloadMemorySpace               = fixture.memorySpace;

  ////////////////////// Test begin

  // Here the channel has been created with a smaller capacity. Hence we should fail the last push,
  // even though the payload buffer has enough space to hold one more token

  // Wait for the consumer 1
  coordinationCommunicationManager.fence(CHANNEL_TAG);
  payloadCommunicationManager.fence(CHANNEL_TAG);

  // Send token one by one
  for (size_t i = 0; i < CHANNEL_CAPACITY - 1; ++i)
  {
    // Prepare slot to send
    ELEMENT_TYPE sendBuffer[1] = {static_cast<ELEMENT_TYPE>(i)};
    auto         sendBufferPtr = &sendBuffer;
    auto         sendSlot      = payloadMemoryManager.registerLocalMemorySlot(payloadMemorySpace, sendBufferPtr, sizeof(sendBuffer));

    // Push and check depths are increased
    ASSERT_EQ(producer.getCoordinationDepth(), i);
    ASSERT_EQ(producer.getPayloadDepth(), i * sizeof(ELEMENT_TYPE));
    ASSERT_NO_THROW(producer.push(sendSlot));

    // Fence to synchronize with the consumer
    coordinationCommunicationManager.fence(CHANNEL_TAG);
    payloadCommunicationManager.fence(CHANNEL_TAG);

    ASSERT_EQ(producer.getCoordinationDepth(), i + 1);
    ASSERT_EQ(producer.getPayloadDepth(), (i + 1) * sizeof(ELEMENT_TYPE));
  }

  // This is the last push, and should fail
  // Prepare slot to send
  ELEMENT_TYPE sendBuffer[1] = {static_cast<ELEMENT_TYPE>(10)};
  auto         sendBufferPtr = &sendBuffer;
  auto         sendSlot      = payloadMemoryManager.registerLocalMemorySlot(payloadMemorySpace, sendBufferPtr, sizeof(sendBuffer));

  // Push and check it fails
  ASSERT_THROW(producer.push(sendSlot), HiCR::RuntimeException);

  // Check the channel is full
  ASSERT_TRUE(producer.isFull(1));

  // Wait for the consumer 2
  coordinationCommunicationManager.fence(CHANNEL_TAG);
  payloadCommunicationManager.fence(CHANNEL_TAG);

  // The consumer is popping, check the depths are updated
  for (size_t i = CHANNEL_CAPACITY - 1; i > 0; --i)
  {
    coordinationCommunicationManager.fence(CHANNEL_TAG);
    payloadCommunicationManager.fence(CHANNEL_TAG);
    ASSERT_EQ(producer.getCoordinationDepth(), i - 1);
    ASSERT_EQ(producer.getPayloadDepth(), (i - 1) * sizeof(ELEMENT_TYPE));
  }

  // Wait for the consumer 3
  coordinationCommunicationManager.fence(CHANNEL_TAG);
  payloadCommunicationManager.fence(CHANNEL_TAG);
}

void consumerFc(ChannelFixture &fixture)
{
  // Create producer and pick managers from the fixture
  fixture.consumer = fixture.createConsumer(
    *fixture.memoryManager, *fixture.memoryManager, *fixture.communicationManager, *fixture.communicationManager, fixture.memorySpace, fixture.memorySpace, CHANNEL_CAPACITY - 1);

  auto &consumer                         = *fixture.consumer;
  auto &coordinationCommunicationManager = *fixture.communicationManager;
  auto &payloadCommunicationManager      = *fixture.communicationManager;

  ////////////////////// Test begin

  // Wait for producer 1
  coordinationCommunicationManager.fence(CHANNEL_TAG);
  payloadCommunicationManager.fence(CHANNEL_TAG);

  // The producer is pushing, check depths are increasing
  for (size_t i = 0; i < CHANNEL_CAPACITY - 1; ++i)
  {
    coordinationCommunicationManager.fence(CHANNEL_TAG);
    payloadCommunicationManager.fence(CHANNEL_TAG);

    ASSERT_EQ(consumer.getCoordinationDepth(), i + 1);
    ASSERT_EQ(consumer.getPayloadDepth(), (i + 1) * sizeof(ELEMENT_TYPE));
  }

  // Check channel is full
  ASSERT_TRUE(consumer.isFull(1));

  // Wait for the producer 2
  coordinationCommunicationManager.fence(CHANNEL_TAG);
  payloadCommunicationManager.fence(CHANNEL_TAG);

  // Pop token one by one
  auto peekIndex = 0;
  for (size_t i = CHANNEL_CAPACITY - 1; i > 0; --i)
  {
    // Check depths are decreasing
    ASSERT_EQ(consumer.getCoordinationDepth(), i);
    ASSERT_EQ(consumer.getPayloadDepth(), i * sizeof(ELEMENT_TYPE));

    // Peek and check data are correct
    auto res = consumer.peek();
    ASSERT_EQ(res[0], peekIndex * sizeof(ELEMENT_TYPE));
    ASSERT_EQ(res[1], sizeof(ELEMENT_TYPE));

    auto  tokenBuffer = (uint8_t *)consumer.getPayloadBufferMemorySlot()->getSourceLocalMemorySlot()->getPointer();
    void *tokenPtr    = &tokenBuffer[res[0]];
    auto  token       = static_cast<ELEMENT_TYPE *>(tokenPtr)[0];
    // In this specific test, the value pushed coincides with the index in the channel
    ASSERT_EQ(peekIndex, token);

    // Pop
    consumer.pop();
    coordinationCommunicationManager.fence(CHANNEL_TAG);
    payloadCommunicationManager.fence(CHANNEL_TAG);

    ASSERT_EQ(consumer.getCoordinationDepth(), i - 1);
    ASSERT_EQ(consumer.getPayloadDepth(), (i - 1) * sizeof(ELEMENT_TYPE));

    // Update peek index
    peekIndex++;
  }

  // Wait for the producer 3
  coordinationCommunicationManager.fence(CHANNEL_TAG);
  payloadCommunicationManager.fence(CHANNEL_TAG);
}

TEST_F(ChannelFixture, FillCoordinationBuffer)
{
  // Rank 0 is producer, Rank 1 is consumer
  if (instanceManager->getCurrentInstance()->isRootInstance()) { producerFc(*this); }
  else { consumerFc(*this); }
}
