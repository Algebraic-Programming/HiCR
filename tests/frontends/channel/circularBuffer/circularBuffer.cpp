#include <gtest/gtest.h>

#include <hicr/core/definitions.hpp>
#include <hicr/frontends/channel/circularBuffer.hpp>

#define BUFFER_CAPACITY 5

TEST(CircularBuffer, isEmpty)
{
  size_t headCounter = 0;
  size_t tailCounter = 0;
  auto   b           = HiCR::channel::CircularBuffer(BUFFER_CAPACITY, &headCounter, &tailCounter);

  ASSERT_TRUE(b.isEmpty());
}

TEST(CircularBuffer, advanceHeadTail)
{
  size_t headCounter = 0;
  size_t tailCounter = 0;
  auto   b           = HiCR::channel::CircularBuffer(BUFFER_CAPACITY, &headCounter, &tailCounter);

  ASSERT_EQ(b.getHeadPosition(), 0);
  ASSERT_EQ(b.getTailPosition(), 0);

  b.advanceHead(2);
  ASSERT_EQ(b.getHeadPosition(), 2);
  ASSERT_EQ(b.getTailPosition(), 0);

  b.advanceTail(2);
  ASSERT_EQ(b.getHeadPosition(), 2);
  ASSERT_EQ(b.getTailPosition(), 2);
}

TEST(CircularBuffer, advanceTailFail)
{
  size_t headCounter = 0;
  size_t tailCounter = 0;
  auto   b           = HiCR::channel::CircularBuffer(BUFFER_CAPACITY, &headCounter, &tailCounter);

  ASSERT_EQ(b.getTailPosition(), 0);
  ASSERT_THROW(b.advanceTail(2), HiCR::FatalException);
}

TEST(CircularBuffer, advanceOverCapacity)
{
  size_t headCounter = 0;
  size_t tailCounter = 0;
  auto   b           = HiCR::channel::CircularBuffer(BUFFER_CAPACITY, &headCounter, &tailCounter);

  for (size_t i = 0; i < BUFFER_CAPACITY * 2; i++)
  {
    EXPECT_NO_THROW(b.advanceHead());
    EXPECT_NO_THROW(b.advanceTail());
  }

  ASSERT_EQ(b.getHeadPosition(), (BUFFER_CAPACITY * 2) % BUFFER_CAPACITY);
  ASSERT_EQ(b.getTailPosition(), (BUFFER_CAPACITY * 2) % BUFFER_CAPACITY);
}

TEST(CircularBuffer, advanceOverCapacityFail)
{
  size_t headCounter = 0;
  size_t tailCounter = 0;
  auto   b           = HiCR::channel::CircularBuffer(BUFFER_CAPACITY, &headCounter, &tailCounter);

  EXPECT_THROW(b.advanceHead(BUFFER_CAPACITY * 2), HiCR::FatalException);
  EXPECT_THROW(b.advanceTail(BUFFER_CAPACITY * 2), HiCR::FatalException);

  for (size_t i = 0; i < BUFFER_CAPACITY * 2; i++)
  {
    if (i < BUFFER_CAPACITY) { EXPECT_NO_THROW(b.advanceHead()); }
    else { EXPECT_THROW(b.advanceHead(), HiCR::FatalException); }
  }
}

TEST(CircularBuffer, getDepth)
{
  size_t headCounter = 0;
  size_t tailCounter = 0;
  auto   b           = HiCR::channel::CircularBuffer(BUFFER_CAPACITY, &headCounter, &tailCounter);

  ASSERT_EQ(b.getDepth(), 0);

  b.advanceHead(BUFFER_CAPACITY);

  for (size_t i = 0; i < BUFFER_CAPACITY; i++)
  {
    ASSERT_EQ(b.getDepth(), BUFFER_CAPACITY - i);
    b.advanceTail();
  }

  b.advanceHead(BUFFER_CAPACITY);

  for (size_t i = 0; i < BUFFER_CAPACITY; i++)
  {
    ASSERT_EQ(b.getDepth(), BUFFER_CAPACITY - i);
    b.advanceTail();
  }
}

TEST(CircularBuffer, isFull)
{
  size_t headCounter = 0;
  size_t tailCounter = 0;
  auto   b           = HiCR::channel::CircularBuffer(BUFFER_CAPACITY, &headCounter, &tailCounter);

  for (size_t i = 0; i < BUFFER_CAPACITY; i++) { b.advanceHead(); }

  ASSERT_TRUE(b.isFull());
}