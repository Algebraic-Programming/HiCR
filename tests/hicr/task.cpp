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

TEST(Task, Construction)
{
  HiCR::Task *t = NULL;
  HiCR::Task::taskFunction_t f;

  EXPECT_NO_THROW(t = new HiCR::Task(f, NULL));
  EXPECT_FALSE(t == nullptr);
  delete t;
}

TEST(Task, SetterAndGetters)
{
  HiCR::Task::taskFunction_t f;
  HiCR::Task t(f, NULL);

  auto arg = &t;
  EXPECT_NO_THROW(t.setArgument(arg));
  EXPECT_EQ(t.getArgument(), arg);

  HiCR::Task::taskEventMap_t e;
  EXPECT_NO_THROW(t.setEventMap(&e));
  EXPECT_EQ(t.getEventMap(), &e);

  HiCR::Task::state_t state;
  EXPECT_NO_THROW(state = t.getState());
  EXPECT_EQ(state, HiCR::Task::state_t::initialized);
}

TEST(Task, Run)
{
  // Storage for internal checks in the task
  bool hasRunningState = false;
  bool hasCorrectTaskPointer = false;

  // Creating task function
  auto f = [&hasRunningState, &hasCorrectTaskPointer](void *arg)
  {
    // Recovering task's own pointer
    HiCR::Task *t = (HiCR::Task *)arg;

    // Checking whether the state is correctly assigned
    if (t->getState() == HiCR::Task::state_t::running) hasRunningState = true;

    // Checking whether the current task pointer is the correct one
    if (HiCR::getCurrentTask() == t) hasCorrectTaskPointer = true;

    // Yielding as many times as necessary
    t->yield();
  };

  // Setting function to run and argument
  HiCR::Task t(f);
  t.setArgument(&t);

  // A first run should start the task
  EXPECT_EQ(t.getState(), HiCR::Task::state_t::initialized);
  EXPECT_NO_THROW(t.run());
  EXPECT_TRUE(hasRunningState);
  EXPECT_TRUE(hasCorrectTaskPointer);
  EXPECT_EQ(t.getState(), HiCR::Task::state_t::suspended);
  EXPECT_EQ(HiCR::getCurrentTask(), (HiCR::Task *)NULL);

  // A second run should resume the task
  EXPECT_NO_THROW(t.run());
  EXPECT_EQ(HiCR::getCurrentTask(), (HiCR::Task *)NULL);
  EXPECT_EQ(t.getState(), HiCR::Task::state_t::finished);

  // The task has now finished, so a third run should fail
  EXPECT_THROW(t.run(), HiCR::RuntimeException);
}

TEST(Task, Events)
{
  // Test flags
  bool onExecuteHasRun = false;
  bool onExecuteUpdated = false;
  bool onSuspendHasRun = false;
  bool onFinishHasRun = false;

  // Creating callbacks
  auto onExecuteCallback = [&onExecuteHasRun](HiCR::Task *t)
  { onExecuteHasRun = true; };
  auto onSuspendCallback = [&onSuspendHasRun](HiCR::Task *t)
  { onSuspendHasRun = true; };
  auto onFinishCallback = [&onFinishHasRun](HiCR::Task *t)
  { onFinishHasRun = true; delete t ; };

  // Creating event map
  HiCR::Task::taskEventMap_t eventMap;

  // Associating events to the map
  eventMap.setEvent(HiCR::Task::event_t::onTaskExecute, onExecuteCallback);
  eventMap.setEvent(HiCR::Task::event_t::onTaskSuspend, onSuspendCallback);
  eventMap.setEvent(HiCR::Task::event_t::onTaskFinish,  onFinishCallback);

  // Creating task function
  auto f = [&onExecuteHasRun, &onExecuteUpdated](void *arg)
  {
    // Recovering task's own pointer
    HiCR::Task *t = (HiCR::Task *)arg;

    // Checking on execute flag has updated correctly
    if (onExecuteHasRun == true) onExecuteUpdated = true;

    // Yielding as many times as necessary
    t->yield();
  };

  // Creating a task without an event map to make sure the functions are not ran
  auto taskNoMap = new HiCR::Task(f);

  // Setting function to run and argument
  taskNoMap->setArgument(taskNoMap);

  // Launching task initially
  EXPECT_NO_THROW(taskNoMap->run());
  EXPECT_FALSE(onExecuteHasRun);
  EXPECT_FALSE(onExecuteUpdated);
  EXPECT_FALSE(onSuspendHasRun);
  EXPECT_FALSE(onFinishHasRun);

  // Resuming task
  EXPECT_NO_THROW(taskNoMap->run());
  EXPECT_FALSE(onFinishHasRun);

  // Freeing memory
  EXPECT_EXIT({ delete taskNoMap; fprintf(stderr, "Delete worked"); exit(0); }, ::testing::ExitedWithCode(0), "Delete worked");

  // Creating a task with an event map to make sure the functions are ran
  auto taskWithMap = new HiCR::Task(f);

  // Setting event map
  taskWithMap->setEventMap(&eventMap);

  // Setting function to run and argument
  taskWithMap->setArgument(taskWithMap);

  // Launching task initially
  EXPECT_NO_THROW(taskWithMap->run());
  EXPECT_TRUE(onExecuteHasRun);
  EXPECT_TRUE(onExecuteUpdated);
  EXPECT_TRUE(onSuspendHasRun);
  EXPECT_FALSE(onFinishHasRun);

  // Resuming task
  EXPECT_NO_THROW(taskWithMap->run());
  EXPECT_TRUE(onFinishHasRun);

  // Attempting to re-free memory (should fail catastrophically)
  EXPECT_DEATH_IF_SUPPORTED(delete taskWithMap, "");
}
