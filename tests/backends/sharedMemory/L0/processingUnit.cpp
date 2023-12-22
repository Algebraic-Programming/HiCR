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
#include <backends/sharedMemory/L1/deviceManager.hpp>
#include <set>

namespace backend = HiCR::backend::sharedMemory;

TEST(ProcessingUnit, Construction)
{
  backend::L0::ProcessingUnit *p = NULL;

  // Creating HWloc topology object
  hwloc_topology_t topology;

  // Reserving memory for hwloc
  hwloc_topology_init(&topology);

  // Initializing Sequential backend's device manager
  HiCR::backend::sharedMemory::L1::DeviceManager dm(&topology);

  // Asking backend to check the available devices
  dm.queryDevices();

  // Getting first device found
  auto d = *dm.getDevices().begin();

  // Updating the compute resource list
  auto computeResources = d->getComputeResourceList();

  // Getting first compute resource
  auto computeResource = *computeResources.begin(); 

  EXPECT_NO_THROW(p = new backend::L0::ProcessingUnit(computeResource));
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
  // Creating HWloc topology object
  hwloc_topology_t topology;

  // Reserving memory for hwloc
  hwloc_topology_init(&topology);

  // Instantiating default compute manager
  backend::L1::ComputeManager m;

  // Initializing Sequential backend's device manager
  HiCR::backend::sharedMemory::L1::DeviceManager dm(&topology);

  // Asking backend to check the available devices
  dm.queryDevices();

  // Getting first device found
  auto d = *dm.getDevices().begin();

  // Updating the compute resource list
  auto computeResources = d->getComputeResourceList();

  // Casting compute resource correctly
  auto computeResource = (backend::L0::ComputeResource*) *computeResources.begin();

  // Creating processing unit from resource
  auto processingUnit = m.createProcessingUnit(computeResource);

  // Getting compute unit affinity
  auto threadAffinity = computeResource->getProcessorId();

  // Putting affinity into a set
  std::set<int> threadAffinitySet({threadAffinity});

  // Initializing processing unit
  EXPECT_NO_THROW(processingUnit->initialize());

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

  // Creating execution unit
  auto executionUnit = m.createExecutionUnit(fc);

  // Creating and initializing execution state
  std::unique_ptr<HiCR::L0::ExecutionState> executionState = NULL;
  EXPECT_NO_THROW(executionState = std::move(m.createExecutionState(executionUnit)));

  // Starting execution state execution
  EXPECT_NO_THROW(processingUnit->start(std::move(executionState)));

  // Waiting for the thread to report
  while (checkedAffinity == false)
    ;

  // Checking if the thread's affinity was correctly set
  EXPECT_TRUE(hasCorrectAffinity);

  // Re-terminating
  EXPECT_NO_THROW(processingUnit->terminate());

  // Re-awaiting
  EXPECT_NO_THROW(processingUnit->await());
}

TEST(ProcessingUnit, LifeCycle)
{
  // Creating HWloc topology object
  hwloc_topology_t topology;

  // Reserving memory for hwloc
  hwloc_topology_init(&topology);

  // Instantiating default compute manager
  backend::L1::ComputeManager m;

  // Initializing Sequential backend's device manager
  HiCR::backend::sharedMemory::L1::DeviceManager dm(&topology);

  // Asking backend to check the available devices
  dm.queryDevices();

  // Getting first device found
  auto d = *dm.getDevices().begin();

  // Updating the compute resource list
  auto computeResources = d->getComputeResourceList();

  // Casting compute resource correctly
  auto computeUnit = (backend::L0::ComputeResource*) *computeResources.begin();

  // Creating processing unit from resource
  auto processingUnit = m.createProcessingUnit(computeUnit);

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

  // Creating execution unit
  auto executionUnit1 = new HiCR::backend::sequential::L0::ExecutionUnit(fc1);

  // Creating execution state
  auto executionState1 = m.createExecutionState(executionUnit1);

  // Testing forbidden transitions
  EXPECT_THROW(processingUnit->start(std::move(executionState1)), HiCR::common::RuntimeException);
  EXPECT_THROW(processingUnit->resume(), HiCR::common::RuntimeException);
  EXPECT_THROW(processingUnit->suspend(), HiCR::common::RuntimeException);
  EXPECT_THROW(processingUnit->terminate(), HiCR::common::RuntimeException);
  EXPECT_THROW(processingUnit->await(), HiCR::common::RuntimeException);

  // Initializing
  EXPECT_NO_THROW(processingUnit->initialize());

  // Testing forbidden transitions
  EXPECT_THROW(processingUnit->initialize(), HiCR::common::RuntimeException);
  EXPECT_THROW(processingUnit->resume(), HiCR::common::RuntimeException);
  EXPECT_THROW(processingUnit->suspend(), HiCR::common::RuntimeException);
  EXPECT_THROW(processingUnit->terminate(), HiCR::common::RuntimeException);
  EXPECT_THROW(processingUnit->await(), HiCR::common::RuntimeException);

  // Running
  executionState1 = m.createExecutionState(executionUnit1);
  EXPECT_NO_THROW(processingUnit->start(std::move(executionState1)));

  // Waiting for execution times to update
  pthread_barrier_wait(&barrier);
  EXPECT_EQ(resumeCounter, 1);

  // Testing forbidden transitions
  executionState1 = m.createExecutionState(executionUnit1);
  EXPECT_THROW(processingUnit->initialize(), HiCR::common::RuntimeException);
  EXPECT_THROW(processingUnit->start(std::move(executionState1)), HiCR::common::RuntimeException);
  EXPECT_THROW(processingUnit->resume(), HiCR::common::RuntimeException);

  // Requesting the thread to suspend
  EXPECT_NO_THROW(processingUnit->suspend());

  // Updating suspend flag
  suspendCounter = suspendCounter + 1;

  // Testing forbidden transitions
  executionState1 = m.createExecutionState(executionUnit1);
  EXPECT_THROW(processingUnit->initialize(), HiCR::common::RuntimeException);
  EXPECT_THROW(processingUnit->start(std::move(executionState1)), HiCR::common::RuntimeException);
  EXPECT_THROW(processingUnit->suspend(), HiCR::common::RuntimeException);
  EXPECT_THROW(processingUnit->terminate(), HiCR::common::RuntimeException);

  // Checking resume counter value has not updated (this is probabilistic only)
  sched_yield();
  usleep(150000);
  sched_yield();
  EXPECT_EQ(resumeCounter, 1);

  // Resuming to terminate function
  EXPECT_NO_THROW(processingUnit->resume());

  // Waiting for execution times to update
  pthread_barrier_wait(&barrier);
  EXPECT_EQ(resumeCounter, 2);

  // Testing forbidden transitions
  executionState1 = m.createExecutionState(executionUnit1);
  EXPECT_THROW(processingUnit->initialize(), HiCR::common::RuntimeException);
  EXPECT_THROW(processingUnit->start(std::move(executionState1)), HiCR::common::RuntimeException);
  EXPECT_THROW(processingUnit->resume(), HiCR::common::RuntimeException);

  // Re-suspend
  EXPECT_NO_THROW(processingUnit->suspend());

  // Updating suspend flag and waiting a bit
  suspendCounter = suspendCounter + 1;

  // Checking resume counter value has not updated (this is probabilistic only)
  sched_yield();
  usleep(50000);
  sched_yield();
  EXPECT_EQ(resumeCounter, 2);

  // Re-Resume
  EXPECT_NO_THROW(processingUnit->resume());
  EXPECT_NO_THROW(processingUnit->terminate());

  // Waiting for execution times to update
  pthread_barrier_wait(&barrier);
  EXPECT_EQ(resumeCounter, 3);

  // Terminate
  executionState1 = m.createExecutionState(executionUnit1);
  EXPECT_THROW(processingUnit->initialize(), HiCR::common::RuntimeException);
  EXPECT_THROW(processingUnit->start(std::move(executionState1)), HiCR::common::RuntimeException);
  EXPECT_THROW(processingUnit->resume(), HiCR::common::RuntimeException);
  EXPECT_THROW(processingUnit->suspend(), HiCR::common::RuntimeException);
  EXPECT_THROW(processingUnit->terminate(), HiCR::common::RuntimeException);

  // Awaiting termination
  executionState1 = m.createExecutionState(executionUnit1);
  EXPECT_NO_THROW(processingUnit->await());
  EXPECT_THROW(processingUnit->start(std::move(executionState1)), HiCR::common::RuntimeException);
  EXPECT_THROW(processingUnit->resume(), HiCR::common::RuntimeException);
  EXPECT_THROW(processingUnit->suspend(), HiCR::common::RuntimeException);
  EXPECT_THROW(processingUnit->terminate(), HiCR::common::RuntimeException);

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
  EXPECT_NO_THROW(processingUnit->initialize());

  // Creating and initializing execution state
  auto executionState2 = m.createExecutionState(executionUnit2);

  // Re-running
  EXPECT_NO_THROW(processingUnit->start(std::move(executionState2)));

  // Waiting for resume counter to update
  pthread_barrier_wait(&barrier);
  EXPECT_EQ(resumeCounter, 4);

  // Re-terminating
  EXPECT_NO_THROW(processingUnit->terminate());

  // Re-awaiting
  EXPECT_NO_THROW(processingUnit->await());

  ///////////////// Creating case where the thread runs a function that finishes
  auto fc3 = []() {
  };

  // Creating execution unit
  auto executionUnit3 = new HiCR::backend::sequential::L0::ExecutionUnit(fc3);

  // Creating and initializing execution state
  auto executionState3 = m.createExecutionState(executionUnit3);

  // Reinitializing
  EXPECT_NO_THROW(processingUnit->initialize());

  // Re-running
  EXPECT_NO_THROW(processingUnit->start(std::move(executionState3)));

  // Re-terminating
  EXPECT_NO_THROW(processingUnit->terminate());

  // Re-awaiting
  EXPECT_NO_THROW(processingUnit->await());
}
