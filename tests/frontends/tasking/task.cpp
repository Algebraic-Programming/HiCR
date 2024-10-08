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
#include <hicr/backends/host/hwloc/L1/topologyManager.hpp>
#include <hicr/backends/host/pthreads/L1/computeManager.hpp>
#include <hicr/frontends/tasking/task.hpp>
#include <hicr/frontends/tasking/tasking.hpp>

TEST(Task, Construction)
{
  HiCR::tasking::Task                     *t = NULL;
  std::shared_ptr<HiCR::L0::ExecutionUnit> u(NULL);

  ASSERT_NO_THROW(t = new HiCR::tasking::Task(u, NULL));
  ASSERT_FALSE(t == nullptr);
  delete t;
}

TEST(Task, SetterAndGetters)
{
  std::shared_ptr<HiCR::L0::ExecutionUnit> u(NULL);
  HiCR::tasking::Task                      t(u, NULL);

  HiCR::tasking::Task::taskCallbackMap_t e;
  ASSERT_NO_THROW(t.setCallbackMap(&e));
  ASSERT_EQ(t.getCallbackMap(), &e);

  HiCR::L0::ExecutionState::state_t state;
  ASSERT_NO_THROW(state = t.getState());
  ASSERT_EQ(state, HiCR::L0::ExecutionState::state_t::uninitialized);
}

TEST(Task, Run)
{
  // Creating HWloc topology object
  hwloc_topology_t topology;

  // Reserving memory for hwloc
  hwloc_topology_init(&topology);

  // Storage for internal checks in the task
  bool hasRunningState       = false;
  bool hasCorrectTaskPointer = false;

  // Pointer for the task to create
  HiCR::tasking::Task *t = NULL;

  // Creating task function
  auto f = [&t, &hasRunningState, &hasCorrectTaskPointer](void *arg) {
    // Checking whether the state is correctly assigned
    if (t->getState() == HiCR::L0::ExecutionState::state_t::running) hasRunningState = true;

    // Checking whether the current task pointer is the correct one
    if ((HiCR::tasking::Task *)arg == t) hasCorrectTaskPointer = true;

    // Yielding as many times as necessary
    t->suspend();
  };

  // Instantiating Pthread-based host (CPU) compute manager
  HiCR::backend::host::pthreads::L1::ComputeManager c;

  // Creating execution unit
  auto u = c.createExecutionUnit(f);

  // Creating task
  t = new HiCR::tasking::Task(u);

  // Initializing HWLoc-based host (CPU) topology manager
  HiCR::backend::host::hwloc::L1::TopologyManager tm(&topology);

  // Asking backend to check the available devices
  const auto tp = tm.queryTopology();

  // Getting first device found
  auto d = *tp.getDevices().begin();

  // Updating the compute resource list
  auto computeResources = d->getComputeResourceList();

  // Getting reference to the first compute resource found
  auto firstComputeResource = *computeResources.begin();

  // Creating processing unit from the compute resource
  auto processingUnit = c.createProcessingUnit(firstComputeResource);

  // Initializing processing unit
  processingUnit->initialize();

  // Creating execution state
  auto executionState = c.createExecutionState(u, t);

  // Then initialize the task with the new execution state
  t->initialize(std::move(executionState));

  // A first run should start the task
  ASSERT_EQ(t->getState(), HiCR::L0::ExecutionState::state_t::initialized);
  ASSERT_NO_THROW(t->run());
  ASSERT_TRUE(hasRunningState);
  ASSERT_TRUE(hasCorrectTaskPointer);
  ASSERT_EQ(t->getState(), HiCR::L0::ExecutionState::state_t::suspended);

  // A second run should resume the task
  ASSERT_NO_THROW(t->run());
  ASSERT_EQ(t->getState(), HiCR::L0::ExecutionState::state_t::finished);

  // The task has now finished, so a third run should fail
  ASSERT_THROW(t->run(), HiCR::RuntimeException);
}

TEST(Task, Callbacks)
{
  // Creating HWloc topology object
  hwloc_topology_t topology;

  // Reserving memory for hwloc
  hwloc_topology_init(&topology);

  // Test flags
  bool onExecuteHasRun  = false;
  bool onExecuteUpdated = false;
  bool onSuspendHasRun  = false;
  bool onFinishHasRun   = false;

  // Creating callbacks
  auto onExecuteCallback = [&onExecuteHasRun](HiCR::tasking::Task *t) { onExecuteHasRun = true; };
  auto onSuspendCallback = [&onSuspendHasRun](HiCR::tasking::Task *t) { onSuspendHasRun = true; };
  auto onFinishCallback  = [&onFinishHasRun](HiCR::tasking::Task *t) {
    onFinishHasRun = true;
    delete t;
  };

  // Creating callback map
  HiCR::tasking::Task::taskCallbackMap_t callbackMap;

  // Associating callbacks to the map
  callbackMap.setCallback(HiCR::tasking::Task::callback_t::onTaskExecute, onExecuteCallback);
  callbackMap.setCallback(HiCR::tasking::Task::callback_t::onTaskSuspend, onSuspendCallback);
  callbackMap.setCallback(HiCR::tasking::Task::callback_t::onTaskFinish, onFinishCallback);

  // Declaring task pointer
  HiCR::tasking::Task *t = NULL;

  // Creating task function
  auto f = [&t, &onExecuteHasRun, &onExecuteUpdated](void *arg) {
    // Checking on execute flag has updated correctly
    if (onExecuteHasRun == true) onExecuteUpdated = true;

    // Yielding as many times as necessary
    t->suspend();
  };

  // Instantiating Pthread-based host (CPU) compute manager
  HiCR::backend::host::pthreads::L1::ComputeManager c;

  // Creating execution unit
  auto u = c.createExecutionUnit(f);

  // Creating task
  t = new HiCR::tasking::Task(u);

  // Initializing HWLoc-based host (CPU) topology manager
  HiCR::backend::host::hwloc::L1::TopologyManager tm(&topology);

  // Asking backend to check the available devices
  const auto tp = tm.queryTopology();

  // Getting first device found
  auto d = *tp.getDevices().begin();

  // Updating the compute resource list
  auto computeResources = d->getComputeResourceList();

  // Getting reference to the first compute resource found
  auto firstComputeResource = *computeResources.begin();

  // Creating processing unit from the compute resource
  auto processingUnit = c.createProcessingUnit(firstComputeResource);

  // Creating execution state
  auto executionState = c.createExecutionState(u);

  // Initializing processing unit
  processingUnit->initialize();

  // Then initialize the task with the new execution state
  t->initialize(std::move(executionState));

  // Launching task initially
  ASSERT_NO_THROW(t->run());
  ASSERT_FALSE(onExecuteHasRun);
  ASSERT_FALSE(onExecuteUpdated);
  ASSERT_FALSE(onSuspendHasRun);
  ASSERT_FALSE(onFinishHasRun);

  // Resuming task
  ASSERT_NO_THROW(t->run());
  ASSERT_FALSE(onFinishHasRun);

  // Freeing memory
  ASSERT_EXIT(
    {
      delete t;
      fprintf(stderr, "Delete worked");
      exit(0);
    },
    ::testing::ExitedWithCode(0),
    "Delete worked");

  // Creating a task with an callback map to make sure the functions are ran
  t = new HiCR::tasking::Task(u);

  // Creating execution state
  executionState = c.createExecutionState(t->getExecutionUnit());

  // Then initialize the task with the new execution state
  t->initialize(std::move(executionState));

  // Setting callback map
  t->setCallbackMap(&callbackMap);

  // Launching task initially
  ASSERT_NO_THROW(t->run());
  ASSERT_TRUE(onExecuteHasRun);
  ASSERT_TRUE(onExecuteUpdated);
  ASSERT_TRUE(onSuspendHasRun);
  ASSERT_FALSE(onFinishHasRun);

  // Resuming task
  ASSERT_NO_THROW(t->run());
  ASSERT_TRUE(onFinishHasRun);

  // Attempting to re-free memory (should fail catastrophically)
  ASSERT_DEATH_IF_SUPPORTED(delete t, "");
}
