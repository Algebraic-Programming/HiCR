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

  HiCR::taskEventMap_t e;
  EXPECT_NO_THROW(t.setEventMap(&e));
  EXPECT_EQ(t.getEventMap(), &e);

  HiCR::task::state_t state;
  EXPECT_NO_THROW(state = t.getState());
  EXPECT_EQ(state, HiCR::task::initialized);
}

TEST(Task, Run)
{
  HiCR::Task t;

  auto f = [](void* arg)
  {
    HiCR::Task* t = (HiCR::Task*) arg;

    // Yielding as many times as necessary
    t->yield();
  };

  // Setting function to run and argument
  t.setFunction(f);
  t.setArgument(&t);

  // A first run should start the task
  EXPECT_NO_THROW(t.run());

  // A second run should resume the task
  EXPECT_NO_THROW(t.run());

  // The task has now finished, so a third run should fail
  EXPECT_THROW(t.run(), HiCR::LogicException);
}

} // namespace
