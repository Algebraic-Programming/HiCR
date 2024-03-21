/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file processingUnit.cpp
 * @brief Unit tests for the Pthread-based processing unit class
 * @author S. M. Martin
 * @date 12/9/2023
 */

#include "gtest/gtest.h"
#include <set>
#include <memory>
#include <hicr/exceptions.hpp>
#include <backends/host/pthreads/L0/processingUnit.hpp>
#include <backends/host/pthreads/L1/computeManager.hpp>

TEST(ProcessingUnit, Construction)
{
  auto computeResource = HiCR::backend::host::L0::ComputeResource(0, 0, 0, {});
  auto cPtr            = std::make_shared<HiCR::backend::host::L0::ComputeResource>(computeResource);

  HiCR::backend::host::pthreads::L0::ProcessingUnit *p = nullptr;
  ASSERT_NO_THROW(p = new HiCR::backend::host::pthreads::L0::ProcessingUnit(cPtr));
  ASSERT_FALSE(p == nullptr);
  delete p;
}

TEST(ProcessingUnit, AffinityFunctions)
{
  // Storing current affinity set
  std::set<int> originalAffinitySet;
  ASSERT_NO_THROW(originalAffinitySet = HiCR::backend::host::pthreads::L0::ProcessingUnit::getAffinity());

  // Attempting to set and check new affinity set
  std::set<int> newAffinitySet({0, 1});
  ASSERT_NO_THROW(HiCR::backend::host::pthreads::L0::ProcessingUnit::updateAffinity(newAffinitySet));
  ASSERT_EQ(newAffinitySet, HiCR::backend::host::pthreads::L0::ProcessingUnit::getAffinity());

  // Re-setting affinity set
  ASSERT_NO_THROW(HiCR::backend::host::pthreads::L0::ProcessingUnit::updateAffinity(originalAffinitySet));
  ASSERT_EQ(originalAffinitySet, HiCR::backend::host::pthreads::L0::ProcessingUnit::getAffinity());
}

TEST(ProcessingUnit, ThreadAffinity)
{
  // Creating HWloc topology object
  hwloc_topology_t topology;

  // Reserving memory for hwloc
  hwloc_topology_init(&topology);

  // Instantiating default compute manager
  HiCR::backend::host::pthreads::L1::ComputeManager m;

  // Creating compute resource (core) manually
  auto computeResource = HiCR::backend::host::L0::ComputeResource(0, 0, 0, {});
  auto cPtr            = std::make_shared<HiCR::backend::host::L0::ComputeResource>(computeResource);

  // Creating processing unit from resource
  auto processingUnit = m.createProcessingUnit(cPtr);

  // Getting compute unit affinity
  auto threadAffinity = cPtr->getProcessorId();

  // Putting affinity into a set
  std::set<int> threadAffinitySet({threadAffinity});

  // Initializing processing unit
  ASSERT_NO_THROW(processingUnit->initialize());

  __volatile__ bool hasCorrectAffinity = false;
  __volatile__ bool checkedAffinity    = false;

  // Creating affinity checking function
  auto fc = [&hasCorrectAffinity, &checkedAffinity, &threadAffinitySet]() {
    // Getting actual affinity set from the running thread
    auto actualThreadAffinity = HiCR::backend::host::pthreads::L0::ProcessingUnit::getAffinity();

    // Checking whether the one detected corresponds to the resource id
    if (actualThreadAffinity == threadAffinitySet) hasCorrectAffinity = true;

    // Raising checked flag
    checkedAffinity = true;
  };

  // Creating execution unit
  auto executionUnit = m.createExecutionUnit(fc);

  // Creating and initializing execution state
  std::unique_ptr<HiCR::L0::ExecutionState> executionState = NULL;
  ASSERT_NO_THROW(executionState = std::move(m.createExecutionState(executionUnit)));

  // Starting execution state execution
  ASSERT_NO_THROW(processingUnit->start(std::move(executionState)));

  // Waiting for the thread to report
  while (checkedAffinity == false)
    ;

  // Checking if the thread's affinity was correctly set
  ASSERT_TRUE(hasCorrectAffinity);

  // Re-terminating
  ASSERT_NO_THROW(processingUnit->terminate());

  // Re-awaiting
  ASSERT_NO_THROW(processingUnit->await());
}

TEST(ProcessingUnit, LifeCycle)
{
  // Creating HWloc topology object
  hwloc_topology_t topology;

  // Reserving memory for hwloc
  hwloc_topology_init(&topology);

  // Instantiating default compute manager
  HiCR::backend::host::pthreads::L1::ComputeManager m;

  // Creating compute resource (core) manually
  auto computeResource = HiCR::backend::host::L0::ComputeResource(0, 0, 0, {});
  auto cPtr            = std::make_shared<HiCR::backend::host::L0::ComputeResource>(computeResource);

  // Creating processing unit from resource
  auto processingUnit = m.createProcessingUnit(cPtr);

  // Values for correct suspension/resume checking
  __volatile__ int suspendCounter = 0;
  __volatile__ int resumeCounter  = 0;

  // Barrier for synchronization
  pthread_barrier_t barrier;
  pthread_barrier_init(&barrier, NULL, 2);

  // Creating runner function
  auto fc1 = [&resumeCounter, &barrier, &suspendCounter]() {
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

  // Creating execution unit
  auto executionUnit1 = std::make_shared<HiCR::backend::host::L0::ExecutionUnit>(fc1);

  // Creating execution state
  auto executionState1 = m.createExecutionState(executionUnit1);

  // Testing forbidden transitions
  ASSERT_THROW(processingUnit->start(std::move(executionState1)), HiCR::RuntimeException);
  ASSERT_THROW(processingUnit->resume(), HiCR::RuntimeException);
  ASSERT_THROW(processingUnit->suspend(), HiCR::RuntimeException);

  // Initializing
  ASSERT_NO_THROW(processingUnit->initialize());

  // Testing forbidden transitions
  ASSERT_THROW(processingUnit->initialize(), HiCR::RuntimeException);
  ASSERT_THROW(processingUnit->resume(), HiCR::RuntimeException);
  ASSERT_THROW(processingUnit->suspend(), HiCR::RuntimeException);

  // Running
  executionState1 = m.createExecutionState(executionUnit1);
  ASSERT_NO_THROW(processingUnit->start(std::move(executionState1)));

  // Waiting for execution times to update
  pthread_barrier_wait(&barrier);
  ASSERT_EQ(resumeCounter, 1);

  // Testing forbidden transitions
  executionState1 = m.createExecutionState(executionUnit1);
  ASSERT_THROW(processingUnit->initialize(), HiCR::RuntimeException);
  ASSERT_THROW(processingUnit->start(std::move(executionState1)), HiCR::RuntimeException);
  ASSERT_THROW(processingUnit->resume(), HiCR::RuntimeException);

  // Requesting the thread to suspend
  ASSERT_NO_THROW(processingUnit->suspend());

  // Updating suspend flag
  suspendCounter = suspendCounter + 1;

  // Testing forbidden transitions
  executionState1 = m.createExecutionState(executionUnit1);
  ASSERT_THROW(processingUnit->initialize(), HiCR::RuntimeException);
  ASSERT_THROW(processingUnit->start(std::move(executionState1)), HiCR::RuntimeException);
  ASSERT_THROW(processingUnit->suspend(), HiCR::RuntimeException);

  // Checking resume counter value has not updated (this is probabilistic only)
  sched_yield();
  usleep(150000);
  sched_yield();
  ASSERT_EQ(resumeCounter, 1);

  // Resuming to terminate function
  ASSERT_NO_THROW(processingUnit->resume());

  // Waiting for execution times to update
  pthread_barrier_wait(&barrier);
  ASSERT_EQ(resumeCounter, 2);

  // Testing forbidden transitions
  executionState1 = m.createExecutionState(executionUnit1);
  ASSERT_THROW(processingUnit->initialize(), HiCR::RuntimeException);
  ASSERT_THROW(processingUnit->start(std::move(executionState1)), HiCR::RuntimeException);
  ASSERT_THROW(processingUnit->resume(), HiCR::RuntimeException);

  // Re-suspend
  ASSERT_NO_THROW(processingUnit->suspend());

  // Updating suspend flag and waiting a bit
  suspendCounter = suspendCounter + 1;

  // Checking resume counter value has not updated (this is probabilistic only)
  sched_yield();
  usleep(50000);
  sched_yield();
  ASSERT_EQ(resumeCounter, 2);

  // Re-Resume
  ASSERT_NO_THROW(processingUnit->resume());
  ASSERT_NO_THROW(processingUnit->terminate());

  // Waiting for execution times to update
  pthread_barrier_wait(&barrier);
  ASSERT_EQ(resumeCounter, 3);

  // Terminate
  executionState1 = m.createExecutionState(executionUnit1);
  ASSERT_THROW(processingUnit->initialize(), HiCR::RuntimeException);
  ASSERT_THROW(processingUnit->start(std::move(executionState1)), HiCR::RuntimeException);
  ASSERT_THROW(processingUnit->resume(), HiCR::RuntimeException);

  // Awaiting termination
  executionState1 = m.createExecutionState(executionUnit1);
  ASSERT_NO_THROW(processingUnit->await());
  ASSERT_THROW(processingUnit->start(std::move(executionState1)), HiCR::RuntimeException);
  ASSERT_THROW(processingUnit->resume(), HiCR::RuntimeException);
  ASSERT_THROW(processingUnit->suspend(), HiCR::RuntimeException);

  ///////// Checking re-run same thread

  // Creating re-runner function
  auto fc2 = [&resumeCounter, &barrier]() {
    // Checking correct execution
    resumeCounter = resumeCounter + 1;
    pthread_barrier_wait(&barrier);
  };

  // Creating execution unit
  auto executionUnit2 = std::make_shared<HiCR::backend::host::L0::ExecutionUnit>(fc2);

  // Reinitializing
  ASSERT_NO_THROW(processingUnit->initialize());

  // Creating and initializing execution state
  auto executionState2 = m.createExecutionState(executionUnit2);

  // Re-running
  ASSERT_NO_THROW(processingUnit->start(std::move(executionState2)));

  // Waiting for resume counter to update
  pthread_barrier_wait(&barrier);
  ASSERT_EQ(resumeCounter, 4);

  // Re-terminating
  ASSERT_NO_THROW(processingUnit->terminate());

  // Re-awaiting
  ASSERT_NO_THROW(processingUnit->await());

  ///////////////// Creating case where the thread runs a function that finishes
  auto fc3 = []() {};

  // Creating execution unit
  auto executionUnit3 = std::make_shared<HiCR::backend::host::L0::ExecutionUnit>(fc3);

  // Creating and initializing execution state
  auto executionState3 = m.createExecutionState(executionUnit3);

  // Reinitializing
  ASSERT_NO_THROW(processingUnit->initialize());

  // Re-running
  ASSERT_NO_THROW(processingUnit->start(std::move(executionState3)));

  // Re-terminating
  ASSERT_NO_THROW(processingUnit->terminate());

  // Re-awaiting
  ASSERT_NO_THROW(processingUnit->await());
}
