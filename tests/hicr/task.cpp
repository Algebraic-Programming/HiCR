/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file task.cpp
 * @brief Unit tests for the task class
 * @author S. M. Martin
 * @date 21/8/2023
 */

#include "gtest/gtest.h"
#include <hicr/task.hpp>

namespace
{

TEST(Task, Construction)
{
  HiCR::Task* t;

  EXPECT_NO_THROW(t = new HiCR::Task());
  EXPECT_FALSE(t == nullptr);
  EXPECT_NO_THROW(delete t);
}

TEST(Task, SetterAndGetters)
{
  HiCR::Task t;
  int testVal = 0;
  HiCR::taskFunction_t f0;
  HiCR::taskFunction_t f1;

  f0 = [](void* arg){*((int*)arg) = 1;};
  EXPECT_NO_THROW(t.setFunction(f0));
  EXPECT_NO_THROW(f1 = t.getFunction());
  EXPECT_NO_THROW(f1(&testVal));
  EXPECT_EQ(testVal, 1);

  auto arg = &t;
  EXPECT_NO_THROW(t.setArgument(arg));
  EXPECT_EQ(t.getArgument(), arg);

  HiCR::EventMap<HiCR::Task> e;
  EXPECT_NO_THROW(t.setEventMap(&e));
  EXPECT_EQ(t.getEventMap(), &e);

  HiCR::task::state_t state;
  EXPECT_NO_THROW(state = t.getState());
  EXPECT_EQ(state, HiCR::task::ready);
}

TEST(Task, Run)
{
  HiCR::Task t;
}

} // namespace
