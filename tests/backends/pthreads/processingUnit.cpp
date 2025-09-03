/*
 *   Copyright 2025 Huawei Technologies Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file processingUnit.cpp
 * @brief Unit tests for the Pthread-based processing unit class
 * @author S. M. Martin
 * @date 12/9/2023
 */

#include <set>
#include <memory>
#include "gtest/gtest.h"
#include <hicr/core/exceptions.hpp>
#include <hicr/backends/pthreads/processingUnit.hpp>
#include <hicr/backends/pthreads/computeManager.hpp>

TEST(ProcessingUnit, Construction)
{
  auto computeResource = HiCR::backend::hwloc::ComputeResource(0, 0, 0, 0, {});
  auto cPtr            = std::make_shared<HiCR::backend::hwloc::ComputeResource>(computeResource);

  HiCR::backend::pthreads::ProcessingUnit *p = nullptr;
  ASSERT_NO_THROW(p = new HiCR::backend::pthreads::ProcessingUnit(cPtr));
  ASSERT_FALSE(p == nullptr);
  delete p;
}

TEST(ProcessingUnit, AffinityFunctions)
{
  // Storing current affinity set
  std::set<HiCR::backend::hwloc::ComputeResource::logicalProcessorId_t> originalAffinitySet;
  ASSERT_NO_THROW(originalAffinitySet = HiCR::backend::pthreads::ProcessingUnit::getAffinity());

  // Attempting to set and check new affinity set
  std::set<HiCR::backend::hwloc::ComputeResource::logicalProcessorId_t> newAffinitySet({0, 1});
  ASSERT_NO_THROW(HiCR::backend::pthreads::ProcessingUnit::updateAffinity(newAffinitySet));
  ASSERT_EQ(newAffinitySet, HiCR::backend::pthreads::ProcessingUnit::getAffinity());

  // Re-setting affinity set
  ASSERT_NO_THROW(HiCR::backend::pthreads::ProcessingUnit::updateAffinity(originalAffinitySet));
  ASSERT_EQ(originalAffinitySet, HiCR::backend::pthreads::ProcessingUnit::getAffinity());
}

TEST(ProcessingUnit, ThreadAffinity)
{
  // Creating HWloc topology object
  hwloc_topology_t topology;

  // Reserving memory for hwloc
  hwloc_topology_init(&topology);

  // Instantiating default compute manager
  HiCR::backend::pthreads::ComputeManager m;

  // Creating compute resource (core) manually
  auto computeResource = HiCR::backend::hwloc::ComputeResource(0, 0, 0, 0, {});
  auto cPtr            = std::make_shared<HiCR::backend::hwloc::ComputeResource>(computeResource);

  // Creating processing unit from resource
  auto processingUnit = m.createProcessingUnit(cPtr);

  // Getting compute unit affinity
  auto threadAffinity = cPtr->getProcessorId();

  // Putting affinity into a set
  std::set<HiCR::backend::hwloc::ComputeResource::logicalProcessorId_t> threadAffinitySet({threadAffinity});

  // Initializing processing unit
  ASSERT_NO_THROW(m.initialize(processingUnit));

  __volatile__ bool hasCorrectAffinity = false;
  __volatile__ bool checkedAffinity    = false;

  // Creating affinity checking function
  auto fc = [&hasCorrectAffinity, &checkedAffinity, &threadAffinitySet](void *arg) {
    // Getting actual affinity set from the running thread
    auto actualThreadAffinity = HiCR::backend::pthreads::ProcessingUnit::getAffinity();

    // Checking whether the one detected corresponds to the resource id
    if (actualThreadAffinity == threadAffinitySet) hasCorrectAffinity = true;

    // Raising checked flag
    checkedAffinity = true;
  };

  // Creating execution unit
  auto executionUnit = m.createExecutionUnit(fc);

  // Creating and initializing execution state
  std::unique_ptr<HiCR::ExecutionState> executionState = NULL;
  ASSERT_NO_THROW(executionState = m.createExecutionState(executionUnit));

  // Starting execution state execution
  ASSERT_NO_THROW(m.start(processingUnit, executionState));

  // Waiting for the thread to report
  while (checkedAffinity == false);

  // Checking if the thread's affinity was correctly set
  ASSERT_TRUE(hasCorrectAffinity);

  // Re-terminating
  ASSERT_NO_THROW(m.terminate(processingUnit));

  // Re-awaiting
  ASSERT_NO_THROW(m.await(processingUnit));
}

TEST(ProcessingUnit, LifeCycle)
{
  // Creating HWloc topology object
  hwloc_topology_t topology;

  // Reserving memory for hwloc
  hwloc_topology_init(&topology);

  // Instantiating default compute manager
  HiCR::backend::pthreads::ComputeManager m;

  // Creating compute resource (core) manually
  auto computeResource = HiCR::backend::hwloc::ComputeResource(0, 0, 0, 0, {});
  auto cPtr            = std::make_shared<HiCR::backend::hwloc::ComputeResource>(computeResource);

  // Creating processing unit from resource
  auto processingUnit = m.createProcessingUnit(cPtr);

  // Values for correct suspension/resume checking
  __volatile__ int suspendCounter = 0;
  __volatile__ int resumeCounter  = 0;

  // Barrier for synchronization
  pthread_barrier_t barrier;
  pthread_barrier_init(&barrier, NULL, 2);

  // Creating runner function
  auto fc1 = [&resumeCounter, &barrier, &suspendCounter](void *arg) {
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
  auto executionUnit1 = std::make_shared<HiCR::backend::pthreads::ExecutionUnit>(fc1);

  // Creating execution state
  auto executionState1 = m.createExecutionState(executionUnit1);

  // Testing forbidden transitions
  ASSERT_THROW(m.start(processingUnit, executionState1), HiCR::RuntimeException);
  ASSERT_THROW(m.resume(processingUnit), HiCR::RuntimeException);
  ASSERT_THROW(m.suspend(processingUnit), HiCR::RuntimeException);

  // Initializing
  ASSERT_NO_THROW(m.initialize(processingUnit));

  // Testing forbidden transitions
  ASSERT_THROW(m.initialize(processingUnit), HiCR::RuntimeException);
  ASSERT_THROW(m.resume(processingUnit), HiCR::RuntimeException);
  ASSERT_THROW(m.suspend(processingUnit), HiCR::RuntimeException);

  // Running
  executionState1 = m.createExecutionState(executionUnit1);
  ASSERT_NO_THROW(m.start(processingUnit, executionState1));

  // Waiting for execution times to update
  pthread_barrier_wait(&barrier);
  ASSERT_EQ(resumeCounter, 1);

  // Testing forbidden transitions
  executionState1 = m.createExecutionState(executionUnit1);
  ASSERT_THROW(m.initialize(processingUnit), HiCR::RuntimeException);
  ASSERT_THROW(m.start(processingUnit, executionState1), HiCR::RuntimeException);
  ASSERT_THROW(m.resume(processingUnit), HiCR::RuntimeException);

  // Requesting the thread to suspend
  ASSERT_NO_THROW(m.suspend(processingUnit));

  // Updating suspend flag
  suspendCounter = suspendCounter + 1;

  // Testing forbidden transitions
  executionState1 = m.createExecutionState(executionUnit1);
  ASSERT_THROW(m.initialize(processingUnit), HiCR::RuntimeException);
  ASSERT_THROW(m.start(processingUnit, executionState1), HiCR::RuntimeException);
  ASSERT_THROW(m.suspend(processingUnit), HiCR::RuntimeException);

  // Checking resume counter value has not updated (this is probabilistic only)
  sched_yield();
  usleep(150000);
  sched_yield();
  ASSERT_EQ(resumeCounter, 1);

  // Resuming to terminate function
  ASSERT_NO_THROW(m.resume(processingUnit));

  // Waiting for execution times to update
  pthread_barrier_wait(&barrier);
  ASSERT_EQ(resumeCounter, 2);

  // Testing forbidden transitions
  executionState1 = m.createExecutionState(executionUnit1);
  ASSERT_THROW(m.initialize(processingUnit), HiCR::RuntimeException);
  ASSERT_THROW(m.start(processingUnit, executionState1), HiCR::RuntimeException);
  ASSERT_THROW(m.resume(processingUnit), HiCR::RuntimeException);

  // Re-suspend
  ASSERT_NO_THROW(m.suspend(processingUnit));

  // Updating suspend flag and waiting a bit
  suspendCounter = suspendCounter + 1;

  // Checking resume counter value has not updated (this is probabilistic only)
  sched_yield();
  usleep(50000);
  sched_yield();
  ASSERT_EQ(resumeCounter, 2);

  // Re-Resume
  ASSERT_NO_THROW(m.resume(processingUnit));
  ASSERT_NO_THROW(m.terminate(processingUnit));

  // Waiting for execution times to update
  pthread_barrier_wait(&barrier);
  ASSERT_EQ(resumeCounter, 3);

  // Terminate
  executionState1 = m.createExecutionState(executionUnit1);
  ASSERT_THROW(m.initialize(processingUnit), HiCR::RuntimeException);
  ASSERT_THROW(m.start(processingUnit, executionState1), HiCR::RuntimeException);
  ASSERT_THROW(m.resume(processingUnit), HiCR::RuntimeException);

  // Awaiting termination
  executionState1 = m.createExecutionState(executionUnit1);
  ASSERT_NO_THROW(m.await(processingUnit));
  ASSERT_THROW(m.start(processingUnit, executionState1), HiCR::RuntimeException);
  ASSERT_THROW(m.resume(processingUnit), HiCR::RuntimeException);
  ASSERT_THROW(m.suspend(processingUnit), HiCR::RuntimeException);

  ///////// Checking re-run same thread

  // Creating re-runner function
  auto fc2 = [&resumeCounter, &barrier](void *arg) {
    // Checking correct execution
    resumeCounter = resumeCounter + 1;
    pthread_barrier_wait(&barrier);
  };

  // Creating execution unit
  auto executionUnit2 = std::make_shared<HiCR::backend::pthreads::ExecutionUnit>(fc2);

  // Reinitializing
  ASSERT_NO_THROW(m.initialize(processingUnit));

  // Creating and initializing execution state
  auto executionState2 = m.createExecutionState(executionUnit2);

  // Re-running
  ASSERT_NO_THROW(m.start(processingUnit, executionState2));

  // Waiting for resume counter to update
  pthread_barrier_wait(&barrier);
  ASSERT_EQ(resumeCounter, 4);

  // Re-terminating
  ASSERT_NO_THROW(m.terminate(processingUnit));

  // Re-awaiting
  ASSERT_NO_THROW(m.await(processingUnit));

  ///////////////// Creating case where the thread runs a function that finishes
  auto fc3 = [](void *arg) {};

  // Creating execution unit
  auto executionUnit3 = std::make_shared<HiCR::backend::pthreads::ExecutionUnit>(fc3);

  // Creating and initializing execution state
  auto executionState3 = m.createExecutionState(executionUnit3);

  // Reinitializing
  ASSERT_NO_THROW(m.initialize(processingUnit));

  // Re-running
  ASSERT_NO_THROW(m.start(processingUnit, executionState3));

  // Re-terminating
  ASSERT_NO_THROW(m.terminate(processingUnit));

  // Re-awaiting
  ASSERT_NO_THROW(m.await(processingUnit));
}
