/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file processingUnit.cpp
 * @brief Unit tests for the shared memory processing unit class
 * @author S. M. Martin
 * @date 12/9/2023
 */

#include "gtest/gtest.h"
#include <backends/sharedMemory/L0/processingUnit.hpp>
#include <backends/sharedMemory/L1/computeManager.hpp>
#include <set>

namespace backend = HiCR::backend::sharedMemory;

TEST(ProcessingUnit, Construction)
{
  backend::L0::ProcessingUnit *p = NULL;

  EXPECT_NO_THROW(p = new backend::L0::ProcessingUnit(0));
  EXPECT_FALSE(p == nullptr);
  delete p;
}

TEST(ProcessingUnit, AffinityFunctions)
{
  // Storing current affinity set
  std::set<int> originalAffinitySet;
  EXPECT_NO_THROW(originalAffinitySet = backend::L0::ProcessingUnit::getAffinity());

  // Attempting to set and check new affinity set
  std::set<int> newAffinitySet({0, 1});
  EXPECT_NO_THROW(backend::L0::ProcessingUnit::updateAffinity(newAffinitySet));
  EXPECT_EQ(newAffinitySet, backend::L0::ProcessingUnit::getAffinity());

  // Re-setting affinity set
  EXPECT_NO_THROW(backend::L0::ProcessingUnit::updateAffinity(originalAffinitySet));
  EXPECT_EQ(originalAffinitySet, backend::L0::ProcessingUnit::getAffinity());
}

TEST(ProcessingUnit, ThreadAffinity)
{
  // Checking that a created thread has a correct affinity
  int threadAffinity = 1;
  std::set<int> threadAffinitySet({threadAffinity});
  backend::L0::ProcessingUnit p(threadAffinity);

  // Initializing processing unit
  EXPECT_NO_THROW(p.initialize());

  __volatile__ bool hasCorrectAffinity = false;
  __volatile__ bool checkedAffinity = false;

  // Creating affinity checking function
  auto fc = [&hasCorrectAffinity, &checkedAffinity, &threadAffinitySet]()
  {
    // Getting actual affinity set from the running thread
    auto actualThreadAffinity = backend::L0::ProcessingUnit::getAffinity();

    // Checking whether the one detected corresponds to the resource id
    if (actualThreadAffinity == threadAffinitySet) hasCorrectAffinity = true;

    // Raising checked flag
    checkedAffinity = true;
  };

  // Creating HWloc topology object
  hwloc_topology_t topology;

  // Reserving memory for hwloc
  hwloc_topology_init(&topology);

  // Creating compute manager
  HiCR::backend::sharedMemory::L1::ComputeManager m(&topology);

  // Creating execution unit
  auto executionUnit = m.createExecutionUnit(fc);

  // Creating and initializing execution state
  std::unique_ptr<HiCR::L0::ExecutionState> executionState = NULL;
  EXPECT_NO_THROW(executionState = std::move(p.createExecutionState(executionUnit)));

  // Starting execution state execution
  EXPECT_NO_THROW(p.start(std::move(executionState)));

  // Waiting for the thread to report
  while (checkedAffinity == false)
    ;

  // Checking if the thread's affinity was correctly set
  EXPECT_TRUE(hasCorrectAffinity);

  // Re-terminating
  EXPECT_NO_THROW(p.terminate());

  // Re-awaiting
  EXPECT_NO_THROW(p.await());
}

TEST(ProcessingUnit, LifeCycle)
{
  HiCR::L0::computeResourceId_t pId = 0;
  backend::L0::ProcessingUnit p(pId);

  // Checking that the correct resourceId was used
  HiCR::L0::computeResourceId_t pIdAlt = pId + 1;
  EXPECT_NO_THROW(pIdAlt = p.getComputeResourceId());
  EXPECT_EQ(pIdAlt, pId);

  // Values for correct suspension/resume checking
  __volatile__ int suspendCounter = 0;
  __volatile__ int resumeCounter = 0;

  // Barrier for synchronization
  pthread_barrier_t barrier;
  pthread_barrier_init(&barrier, NULL, 2);

  // Creating runner function
  auto fc1 = [&resumeCounter, &barrier, &suspendCounter]()
  {
    // Checking correct execution
    resumeCounter = resumeCounter + 1;
    pthread_barrier_wait(&barrier);

    // Checking suspension
    while (suspendCounter == 0) {};

    // Updating resume counter
    resumeCounter = resumeCounter + 1;
    pthread_barrier_wait(&barrier);

    // Checking suspension
    while (suspendCounter == 1) {};

    // Updating resume counter
    resumeCounter = resumeCounter + 1;

    pthread_barrier_wait(&barrier);
  };

  // Creating HWloc topology object
  hwloc_topology_t topology;

  // Reserving memory for hwloc
  hwloc_topology_init(&topology);

  // Creating compute manager
  HiCR::backend::sharedMemory::L1::ComputeManager m(&topology);

  // Creating execution unit
  auto executionUnit1 = new HiCR::backend::sequential::L0::ExecutionUnit(fc1);

  // Testing forbidden transitions
  EXPECT_THROW(p.start(std::move(p.createExecutionState(executionUnit1))), HiCR::common::RuntimeException);
  EXPECT_THROW(p.resume(), HiCR::common::RuntimeException);
  EXPECT_THROW(p.suspend(), HiCR::common::RuntimeException);
  EXPECT_THROW(p.terminate(), HiCR::common::RuntimeException);
  EXPECT_THROW(p.await(), HiCR::common::RuntimeException);

  // Initializing
  EXPECT_NO_THROW(p.initialize());

  // Testing forbidden transitions
  EXPECT_THROW(p.initialize(), HiCR::common::RuntimeException);
  EXPECT_THROW(p.resume(), HiCR::common::RuntimeException);
  EXPECT_THROW(p.suspend(), HiCR::common::RuntimeException);
  EXPECT_THROW(p.terminate(), HiCR::common::RuntimeException);
  EXPECT_THROW(p.await(), HiCR::common::RuntimeException);

  // Running
  auto executionState = p.createExecutionState(executionUnit1);
  EXPECT_NO_THROW(p.start(std::move(executionState)));

  // Waiting for execution times to update
  pthread_barrier_wait(&barrier);
  EXPECT_EQ(resumeCounter, 1);

  // Testing forbidden transitions
  EXPECT_THROW(p.initialize(), HiCR::common::RuntimeException);
  EXPECT_THROW(p.start(std::move(p.createExecutionState(executionUnit1))), HiCR::common::RuntimeException);
  EXPECT_THROW(p.resume(), HiCR::common::RuntimeException);

  // Requesting the thread to suspend
  EXPECT_NO_THROW(p.suspend());

  // Updating suspend flag
  suspendCounter = suspendCounter + 1;

  // Testing forbidden transitions
  EXPECT_THROW(p.initialize(), HiCR::common::RuntimeException);
  EXPECT_THROW(p.start(std::move(p.createExecutionState(executionUnit1))), HiCR::common::RuntimeException);
  EXPECT_THROW(p.suspend(), HiCR::common::RuntimeException);
  EXPECT_THROW(p.terminate(), HiCR::common::RuntimeException);

  // Checking resume counter value has not updated (this is probabilistic only)
  sched_yield();
  usleep(150000);
  sched_yield();
  EXPECT_EQ(resumeCounter, 1);

  // Resuming to terminate function
  EXPECT_NO_THROW(p.resume());

  // Waiting for execution times to update
  pthread_barrier_wait(&barrier);
  EXPECT_EQ(resumeCounter, 2);

  // Testing forbidden transitions
  EXPECT_THROW(p.initialize(), HiCR::common::RuntimeException);
  EXPECT_THROW(p.start(std::move(p.createExecutionState(executionUnit1))), HiCR::common::RuntimeException);
  EXPECT_THROW(p.resume(), HiCR::common::RuntimeException);

  // Re-suspend
  EXPECT_NO_THROW(p.suspend());

  // Updating suspend flag and waiting a bit
  suspendCounter = suspendCounter + 1;

  // Checking resume counter value has not updated (this is probabilistic only)
  sched_yield();
  usleep(50000);
  sched_yield();
  EXPECT_EQ(resumeCounter, 2);

  // Re-Resume
  EXPECT_NO_THROW(p.resume());
  EXPECT_NO_THROW(p.terminate());

  // Waiting for execution times to update
  pthread_barrier_wait(&barrier);
  EXPECT_EQ(resumeCounter, 3);

  // Terminate
  EXPECT_THROW(p.initialize(), HiCR::common::RuntimeException);
  EXPECT_THROW(p.start(std::move(p.createExecutionState(executionUnit1))), HiCR::common::RuntimeException);
  EXPECT_THROW(p.resume(), HiCR::common::RuntimeException);
  EXPECT_THROW(p.suspend(), HiCR::common::RuntimeException);
  EXPECT_THROW(p.terminate(), HiCR::common::RuntimeException);

  // Awaiting termination
  EXPECT_NO_THROW(p.await());
  EXPECT_THROW(p.start(std::move(p.createExecutionState(executionUnit1))), HiCR::common::RuntimeException);
  EXPECT_THROW(p.resume(), HiCR::common::RuntimeException);
  EXPECT_THROW(p.suspend(), HiCR::common::RuntimeException);
  EXPECT_THROW(p.terminate(), HiCR::common::RuntimeException);

  ///////// Checking re-run same thread

  // Creating re-runner function
  auto fc2 = [&resumeCounter, &barrier]()
  {
    // Checking correct execution
    resumeCounter = resumeCounter + 1;
    pthread_barrier_wait(&barrier);
  };

  // Creating execution unit
  auto executionUnit2 = new HiCR::backend::sequential::L0::ExecutionUnit(fc2);

  // Reinitializing
  EXPECT_NO_THROW(p.initialize());

  // Creating and initializing execution state
  auto executionState2 = p.createExecutionState(executionUnit2);

  // Re-running
  EXPECT_NO_THROW(p.start(std::move(executionState2)));

  // Waiting for resume counter to update
  pthread_barrier_wait(&barrier);
  EXPECT_EQ(resumeCounter, 4);

  // Re-terminating
  EXPECT_NO_THROW(p.terminate());

  // Re-awaiting
  EXPECT_NO_THROW(p.await());

  ///////////////// Creating case where the thread runs a function that finishes
  auto fc3 = []() {
  };

  // Creating execution unit
  auto executionUnit3 = new HiCR::backend::sequential::L0::ExecutionUnit(fc3);

  // Creating and initializing execution state
  auto executionState3 = p.createExecutionState(executionUnit3);

  // Reinitializing
  EXPECT_NO_THROW(p.initialize());

  // Re-running
  EXPECT_NO_THROW(p.start(std::move(executionState3)));

  // Re-terminating
  EXPECT_NO_THROW(p.terminate());

  // Re-awaiting
  EXPECT_NO_THROW(p.await());
}
