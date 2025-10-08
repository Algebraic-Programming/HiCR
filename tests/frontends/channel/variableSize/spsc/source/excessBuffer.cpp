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
    *fixture.memoryManager, *fixture.memoryManager, *fixture.communicationManager, *fixture.communicationManager, fixture.memorySpace, fixture.memorySpace, CHANNEL_CAPACITY);

  auto &producer                         = *fixture.producer;
  auto &payloadMemoryManager             = *fixture.memoryManager;
  auto &coordinationCommunicationManager = *fixture.communicationManager;
  auto &payloadCommunicationManager      = *fixture.communicationManager;
  auto  payloadMemorySpace               = fixture.memorySpace;

  ////////////////////// Test begin

  // Check payload capacity, that buffer is empty, an thus not full
  ASSERT_EQ(producer.getPayloadCapacity(), CHANNEL_CAPACITY * sizeof(ELEMENT_TYPE));
  producer.updateDepth();
  ASSERT_EQ(producer.getCoordinationDepth(), 0);
  ASSERT_EQ(producer.getPayloadDepth(), 0);
  ASSERT_TRUE(producer.isEmpty());
  ASSERT_FALSE(producer.isFull(CHANNEL_CAPACITY * sizeof(ELEMENT_TYPE)));
  ASSERT_TRUE(producer.isFull(CHANNEL_CAPACITY * sizeof(ELEMENT_TYPE) + 1));

  // Send a buffer big as the buffer channel
  ELEMENT_TYPE sendBuffer[CHANNEL_CAPACITY - 1] = {0, 1, 2, 3};
  auto         sendBufferPtr                    = &sendBuffer;
  auto         sendSlot                         = payloadMemoryManager.registerLocalMemorySlot(payloadMemorySpace, sendBufferPtr, sizeof(sendBuffer));

  // Wait for the consumer 1
  coordinationCommunicationManager.fence(CHANNEL_TAG);
  payloadCommunicationManager.fence(CHANNEL_TAG);

  // Push the slot
  EXPECT_NO_THROW(producer.push(sendSlot));

  // Check that the channel can accept one more element
  ASSERT_FALSE(producer.isFull(sizeof(ELEMENT_TYPE)));
  ASSERT_TRUE(producer.isFull(2 * sizeof(ELEMENT_TYPE)));
  ASSERT_FALSE(producer.isEmpty());

  // Check there is only one token, and the payload depth is equal to the capacity of the buffer minus 1 element
  producer.updateDepth();
  ASSERT_EQ(producer.getCoordinationDepth(), 1);
  ASSERT_EQ(producer.getPayloadDepth(), (CHANNEL_CAPACITY - 1) * sizeof(ELEMENT_TYPE));

  // Check that trying to push another element throws exception since the channel does not have enough space
  EXPECT_THROW(producer.push(sendSlot), HiCR::RuntimeException);

  // Wait for the consumer 2
  coordinationCommunicationManager.fence(CHANNEL_TAG);
  payloadCommunicationManager.fence(CHANNEL_TAG);

  // Let the consumer pop

  // Wait for the consumer 3
  coordinationCommunicationManager.fence(CHANNEL_TAG);
  payloadCommunicationManager.fence(CHANNEL_TAG);

  // Now that consumer has popped it should succeed, and push to the excess buffer
  EXPECT_NO_THROW(producer.push(sendSlot));

  // Wait for the consumer 4
  coordinationCommunicationManager.fence(CHANNEL_TAG);
  payloadCommunicationManager.fence(CHANNEL_TAG);

  // Let the consumer do its part of the test

  // Wait for the consumer 5
  coordinationCommunicationManager.fence(CHANNEL_TAG);
  payloadCommunicationManager.fence(CHANNEL_TAG);
}

void consumerFc(ChannelFixture &fixture)
{
  // Create producer and pick managers from the fixture
  fixture.consumer = fixture.createConsumer(
    *fixture.memoryManager, *fixture.memoryManager, *fixture.communicationManager, *fixture.communicationManager, fixture.memorySpace, fixture.memorySpace, CHANNEL_CAPACITY);

  auto &consumer                         = *fixture.consumer;
  auto &coordinationCommunicationManager = *fixture.communicationManager;
  auto &payloadCommunicationManager      = *fixture.communicationManager;

  ////////////////////// Test begin

  // Check payload capacity, that buffer is empty, an thus not full
  consumer.updateDepth();
  ASSERT_EQ(consumer.getCoordinationDepth(), 0);
  ASSERT_EQ(consumer.getPayloadDepth(), 0);
  ASSERT_TRUE(consumer.isEmpty());
  ASSERT_FALSE(consumer.isFull(CHANNEL_CAPACITY * sizeof(ELEMENT_TYPE)));
  ASSERT_TRUE(consumer.isFull(CHANNEL_CAPACITY * sizeof(ELEMENT_TYPE) + 1));

  // Wait for producer 1
  coordinationCommunicationManager.fence(CHANNEL_TAG);
  payloadCommunicationManager.fence(CHANNEL_TAG);

  // Let the producer do its part of the test

  // Wait for the producer 2
  coordinationCommunicationManager.fence(CHANNEL_TAG);
  payloadCommunicationManager.fence(CHANNEL_TAG);

  // After the push, check there is one token and payload buffer is full
  consumer.updateDepth();
  ASSERT_EQ(consumer.getCoordinationDepth(), 1);
  ASSERT_EQ(consumer.getPayloadDepth(), (CHANNEL_CAPACITY - 1) * sizeof(ELEMENT_TYPE));
  ASSERT_FALSE(consumer.isEmpty());
  // Check that we still have space to push 1 token
  ASSERT_FALSE(consumer.isFull(sizeof(ELEMENT_TYPE)));
  ASSERT_TRUE(consumer.isFull(2 * sizeof(ELEMENT_TYPE)));

  // Peek and check the token data are correct
  auto res = consumer.peek();
  ASSERT_EQ(res[0], 0);
  ASSERT_EQ(res[1], (CHANNEL_CAPACITY - 1) * sizeof(ELEMENT_TYPE));

  // Check the vectory elements corresponds to the ground truth
  auto  tokenBuffer = (uint8_t *)consumer.getPayloadBufferMemorySlot()->getSourceLocalMemorySlot()->getPointer();
  void *tokenPtr    = &tokenBuffer[res[0]];
  for (ELEMENT_TYPE i = 0; i < (res[1] / sizeof(ELEMENT_TYPE)); ++i) { ASSERT_EQ(i, static_cast<ELEMENT_TYPE *>(tokenPtr)[i]); }

  // Pop and check that the channel is empty, the depth are updated
  consumer.pop();
  ASSERT_TRUE(consumer.isEmpty());
  ASSERT_FALSE(consumer.isFull(CHANNEL_CAPACITY * sizeof(ELEMENT_TYPE)));
  ASSERT_EQ(consumer.getCoordinationDepth(), 0);
  ASSERT_EQ(consumer.getPayloadDepth(), 0);

  // Wait for the producer 3
  coordinationCommunicationManager.fence(CHANNEL_TAG);
  payloadCommunicationManager.fence(CHANNEL_TAG);

  // Let the producer push again

  // Wait for the producer 4
  coordinationCommunicationManager.fence(CHANNEL_TAG);
  payloadCommunicationManager.fence(CHANNEL_TAG);

  // Wait for the producer 5
  coordinationCommunicationManager.fence(CHANNEL_TAG);
  payloadCommunicationManager.fence(CHANNEL_TAG);

  // After the push, check there is one token
  consumer.updateDepth();
  ASSERT_EQ(consumer.getCoordinationDepth(), 1);
  ASSERT_EQ(consumer.getPayloadDepth(), (CHANNEL_CAPACITY - 1) * sizeof(ELEMENT_TYPE));
  ASSERT_FALSE(consumer.isEmpty());
  // Check that we still have space to push 1 token
  ASSERT_FALSE(consumer.isFull(sizeof(ELEMENT_TYPE)));
  ASSERT_TRUE(consumer.isFull(2 * sizeof(ELEMENT_TYPE)));

  // Peek and check the token data are correct
  res = consumer.peek();
  ASSERT_EQ(res[0], (CHANNEL_CAPACITY - 1) * sizeof(ELEMENT_TYPE));
  ASSERT_EQ(res[1], (CHANNEL_CAPACITY - 1) * sizeof(ELEMENT_TYPE));

  // Check the vectory elements corresponds to the ground truth
  tokenBuffer = (uint8_t *)consumer.getPayloadBufferMemorySlot()->getSourceLocalMemorySlot()->getPointer();
  tokenPtr    = &tokenBuffer[res[0]];
  for (ELEMENT_TYPE i = 0; i < (res[1] / sizeof(ELEMENT_TYPE)); ++i) { ASSERT_EQ(i, static_cast<ELEMENT_TYPE *>(tokenPtr)[i]); }

  // Pop and check that the channel is empty, the depth are updated
  consumer.pop();
  ASSERT_TRUE(consumer.isEmpty());
  ASSERT_FALSE(consumer.isFull(CHANNEL_CAPACITY * sizeof(ELEMENT_TYPE)));
  ASSERT_EQ(consumer.getCoordinationDepth(), 0);
  ASSERT_EQ(consumer.getPayloadDepth(), 0);
}

TEST_F(ChannelFixture, UseExcessBuffer)
{
  // Rank 0 is producer, Rank 1 is consumer
  if (instanceManager->getCurrentInstance()->isRootInstance()) { producerFc(*this); }
  else { consumerFc(*this); }
}
