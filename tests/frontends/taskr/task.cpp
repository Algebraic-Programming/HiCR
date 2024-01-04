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
#include <backends/sequential/L1/topologyManager.hpp>
#include <backends/sequential/L1/computeManager.hpp>
#include <frontends/taskr/task.hpp>

TEST(Task, Construction)
{
  taskr::Task *t = NULL;
  std::shared_ptr<HiCR::L0::ExecutionUnit> u(NULL);

  EXPECT_NO_THROW(t = new taskr::Task(0, u, NULL));
  EXPECT_FALSE(t == nullptr);
  delete t;
}

TEST(Task, SetterAndGetters)
{
  std::shared_ptr<HiCR::L0::ExecutionUnit> u(NULL);
  taskr::Task t(0, u, NULL);

  taskr::Task::taskEventMap_t e;
  EXPECT_NO_THROW(t.setEventMap(&e));
  EXPECT_EQ(t.getEventMap(), &e);

  HiCR::L0::ExecutionState::state_t state;
  EXPECT_NO_THROW(state = t.getState());
  EXPECT_EQ(state, HiCR::L0::ExecutionState::state_t::uninitialized);
}

TEST(Task, Run)
{
  // Storage for internal checks in the task
  bool hasRunningState = false;
  bool hasCorrectTaskPointer = false;

  // Pointer for the task to create
  taskr::Task *t = NULL;

  // Creating task function
  auto f = [&t, &hasRunningState, &hasCorrectTaskPointer]()
  {
    // Checking whether the state is correctly assigned
    if (t->getState() == HiCR::L0::ExecutionState::state_t::running) hasRunningState = true;

    // Checking whether the current task pointer is the correct one
    if (taskr::Task::getCurrentTask() == t) hasCorrectTaskPointer = true;

    // Yielding as many times as necessary
    t->suspend();
  };

  // Instantiating default compute manager
  HiCR::backend::sequential::L1::ComputeManager m;

  // Creating execution unit
  auto u = m.createExecutionUnit(f);

  // Creating task
  t = new taskr::Task(0, u);

  // Initializing Sequential backend's topology manager
  HiCR::backend::sequential::L1::TopologyManager dm;

  // Asking backend to check the available devices
  dm.queryDevices();

  // Getting first device found
  auto d = *dm.getDevices().begin();

  // Updating the compute resource list
  auto computeResources = d->getComputeResourceList();

  // Getting reference to the first compute resource found
  auto firstComputeResource = *computeResources.begin();

  // Creating processing unit from the compute resource
  auto processingUnit = m.createProcessingUnit(firstComputeResource);

  // Initializing processing unit
  processingUnit->initialize();

  // Creating execution state
  auto executionState = m.createExecutionState(u);

  // Then initialize the task with the new execution state
  t->initialize(std::move(executionState));

  // A first run should start the task
  EXPECT_EQ(t->getState(), HiCR::L0::ExecutionState::state_t::initialized);
  EXPECT_NO_THROW(t->run());
  EXPECT_TRUE(hasRunningState);
  EXPECT_TRUE(hasCorrectTaskPointer);
  EXPECT_EQ(t->getState(), HiCR::L0::ExecutionState::state_t::suspended);
  EXPECT_EQ(taskr::Task::getCurrentTask(), (taskr::Task *)NULL);

  // A second run should resume the task
  EXPECT_NO_THROW(t->run());
  EXPECT_EQ(taskr::Task::getCurrentTask(), (taskr::Task *)NULL);
  EXPECT_EQ(t->getState(), HiCR::L0::ExecutionState::state_t::finished);

  // The task has now finished, so a third run should fail
  EXPECT_THROW(t->run(), HiCR::RuntimeException);
}

TEST(Task, Events)
{
  // Test flags
  bool onExecuteHasRun = false;
  bool onExecuteUpdated = false;
  bool onSuspendHasRun = false;
  bool onFinishHasRun = false;

  // Creating callbacks
  auto onExecuteCallback = [&onExecuteHasRun](taskr::Task *t)
  { onExecuteHasRun = true; };
  auto onSuspendCallback = [&onSuspendHasRun](taskr::Task *t)
  { onSuspendHasRun = true; };
  auto onFinishCallback = [&onFinishHasRun](taskr::Task *t)
  { onFinishHasRun = true; delete t ; };

  // Creating event map
  taskr::Task::taskEventMap_t eventMap;

  // Associating events to the map
  eventMap.setEvent(taskr::Task::event_t::onTaskExecute, onExecuteCallback);
  eventMap.setEvent(taskr::Task::event_t::onTaskSuspend, onSuspendCallback);
  eventMap.setEvent(taskr::Task::event_t::onTaskFinish, onFinishCallback);

  // Declaring task pointer
  taskr::Task *t = NULL;

  // Creating task function
  auto f = [&t, &onExecuteHasRun, &onExecuteUpdated]()
  {
    // Checking on execute flag has updated correctly
    if (onExecuteHasRun == true) onExecuteUpdated = true;

    // Yielding as many times as necessary
    t->suspend();
  };

  // Instantiating default compute manager
  HiCR::backend::sequential::L1::ComputeManager m;

  // Creating execution unit
  auto u = m.createExecutionUnit(f);

  // Creating task
  t = new taskr::Task(0, u);

  // Initializing Sequential backend's device manager
  HiCR::backend::sequential::L1::TopologyManager dm;

  // Asking backend to check the available devices
  dm.queryDevices();

  // Getting first device found
  auto d = *dm.getDevices().begin();

  // Updating the compute resource list
  auto computeResources = d->getComputeResourceList();

  // Getting reference to the first compute resource found
  auto firstComputeResource = *computeResources.begin();

  // Creating processing unit from the compute resource
  auto processingUnit = m.createProcessingUnit(firstComputeResource);

  // Creating execution state
  auto executionState = m.createExecutionState(u);

  // Initializing processing unit
  processingUnit->initialize();

  // Then initialize the task with the new execution state
  t->initialize(std::move(executionState));

  // Launching task initially
  EXPECT_NO_THROW(t->run());
  EXPECT_FALSE(onExecuteHasRun);
  EXPECT_FALSE(onExecuteUpdated);
  EXPECT_FALSE(onSuspendHasRun);
  EXPECT_FALSE(onFinishHasRun);

  // Resuming task
  EXPECT_NO_THROW(t->run());
  EXPECT_FALSE(onFinishHasRun);

  // Freeing memory
  EXPECT_EXIT({ delete t; fprintf(stderr, "Delete worked"); exit(0); }, ::testing::ExitedWithCode(0), "Delete worked");

  // Creating a task with an event map to make sure the functions are ran
  t = new taskr::Task(1, u);

  // Creating execution state
  executionState = m.createExecutionState(t->getExecutionUnit());

  // Then initialize the task with the new execution state
  t->initialize(std::move(executionState));

  // Setting event map
  t->setEventMap(&eventMap);

  // Launching task initially
  EXPECT_NO_THROW(t->run());
  EXPECT_TRUE(onExecuteHasRun);
  EXPECT_TRUE(onExecuteUpdated);
  EXPECT_TRUE(onSuspendHasRun);
  EXPECT_FALSE(onFinishHasRun);

  // Resuming task
  EXPECT_NO_THROW(t->run());
  EXPECT_TRUE(onFinishHasRun);

  // Attempting to re-free memory (should fail catastrophically)
  EXPECT_DEATH_IF_SUPPORTED(delete t, "");
}
