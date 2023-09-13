/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file worker.cpp
 * @brief Unit tests for the shared memory thread class
 * @author S. M. Martin
 * @date 12/9/2023
 */

#include <set>
#include "gtest/gtest.h"
#include <hicr/backends/sharedMemory/thread.hpp>

namespace backend = HiCR::backend::sharedMemory;

TEST(Thread, Construction)
{
  backend::Thread *p = NULL;

  EXPECT_NO_THROW(p = new backend::Thread(0));
  EXPECT_FALSE(p == nullptr);
  delete p;
}

TEST(Thread, AffinityFunctions)
{
 // Storing current affinity set
 std::set<int> originalAffinitySet;
 EXPECT_NO_THROW(originalAffinitySet = backend::Thread::getAffinity());

 // Attempting to set and check new affinity set
 std::set<int> newAffinitySet({0, 1});
 EXPECT_NO_THROW(backend::Thread::updateAffinity(newAffinitySet));
 EXPECT_EQ(newAffinitySet, backend::Thread::getAffinity());

 // Re-setting affinity set
 EXPECT_NO_THROW(backend::Thread::updateAffinity(originalAffinitySet));
 EXPECT_EQ(originalAffinitySet, backend::Thread::getAffinity());
}

TEST(Thread, ThreadAffinity)
{
 // Checking that a created thread has a correct affinity
 int threadAffinity = 1;
 std::set<int> threadAffinitySet({threadAffinity});
 backend::Thread p(threadAffinity);

 bool hasCorrectAffinity = false;
 bool checkedAffinity = false;

 // Creating affinity checking function
 auto fc = [&hasCorrectAffinity, &checkedAffinity, &threadAffinitySet]()
 {
  // Getting actual affinity set from the running thread
  auto actualThreadAffinity = backend::Thread::getAffinity();

  // Checking whether the one detected corresponds to the resource id
  if (actualThreadAffinity == threadAffinitySet) hasCorrectAffinity = true;

  // Raising checked flag
  checkedAffinity = true;

  // Waiting
  while(true);
 };

 // Initializing and running thread
 EXPECT_NO_THROW(p.initialize());
 EXPECT_NO_THROW(p.start(fc));

 // Waiting for the thread to report
 while(checkedAffinity == false);

 // Checking if the thread's affinity was correctly set
 EXPECT_TRUE(hasCorrectAffinity);
}

TEST(Thread, LifeCycle)
{
  HiCR::computeResourceId_t pId = 0;
  backend::Thread p(pId);

  // Checking that the correct resourceId was used
  HiCR::computeResourceId_t pIdAlt = pId + 1;
  EXPECT_NO_THROW(pIdAlt = p.getComputeResourceId());
  EXPECT_EQ(pIdAlt, pId);

  // Values for correct suspension/resume checking
  int suspendCounter = 0;
  int resumeCounter = 0;

  // Barrier for synchronization
  pthread_barrier_t barrier;
  pthread_barrier_init(&barrier, NULL, 2);

  // Creating runner function
  auto fc1 = [&resumeCounter, &barrier, &suspendCounter]()
  {
    // Checking correct execution
    resumeCounter++;
    pthread_barrier_wait(&barrier);

    // Checking suspension
    while(suspendCounter == 0);

    // Updating resume counter
    resumeCounter++;
    pthread_barrier_wait(&barrier);

    // Checking suspension
    while(suspendCounter == 1);

    // Updating resume counter
    resumeCounter++;
    pthread_barrier_wait(&barrier);

    // Waiting until the end
    while(true) pthread_barrier_wait(&barrier);
  };

  // Testing forbidden transitions
  EXPECT_THROW(p.start(fc1), HiCR::RuntimeException);
  EXPECT_THROW(p.resume(), HiCR::RuntimeException);
  EXPECT_THROW(p.suspend(), HiCR::RuntimeException);
  EXPECT_THROW(p.terminate(), HiCR::RuntimeException);
  EXPECT_THROW(p.await(), HiCR::RuntimeException);

  // Initializing
  EXPECT_NO_THROW(p.initialize());

  // Testing forbidden transitions
  EXPECT_THROW(p.initialize(), HiCR::RuntimeException);
  EXPECT_THROW(p.resume(), HiCR::RuntimeException);
  EXPECT_THROW(p.suspend(), HiCR::RuntimeException);
  EXPECT_THROW(p.terminate(), HiCR::RuntimeException);
  EXPECT_THROW(p.await(), HiCR::RuntimeException);

  // Running
  EXPECT_NO_THROW(p.start(fc1));

  // Waiting for execution times to update
  pthread_barrier_wait(&barrier);
  EXPECT_EQ(resumeCounter, 1);

  // Testing forbidden transitions
  EXPECT_THROW(p.initialize(), HiCR::RuntimeException);
  EXPECT_THROW(p.start(fc1), HiCR::RuntimeException);
  EXPECT_THROW(p.resume(), HiCR::RuntimeException);

  // Requesting the thread to suspend
  EXPECT_NO_THROW(p.suspend());

  // Updating suspend flag
  suspendCounter++;

  // Testing forbidden transitions
  EXPECT_THROW(p.initialize(), HiCR::RuntimeException);
  EXPECT_THROW(p.start(fc1), HiCR::RuntimeException);
  EXPECT_THROW(p.suspend(), HiCR::RuntimeException);
  EXPECT_THROW(p.terminate(), HiCR::RuntimeException);

  // Checking resume counter value has not updated (this is probabilistic only)
  sched_yield();
  usleep(50000);
  sched_yield();
  EXPECT_EQ(resumeCounter, 1);

  // Resuming to terminate function
  EXPECT_NO_THROW(p.resume());

  // Waiting for execution times to update
  pthread_barrier_wait(&barrier);
  EXPECT_EQ(resumeCounter, 2);

  // Testing forbidden transitions
  EXPECT_THROW(p.initialize(), HiCR::RuntimeException);
  EXPECT_THROW(p.start(fc1), HiCR::RuntimeException);
  EXPECT_THROW(p.resume(), HiCR::RuntimeException);

  // Re-suspend
  EXPECT_NO_THROW(p.suspend());

  // Updating suspend flag and waiting a bit
  suspendCounter++;

  // Checking resume counter value has not updated (this is probabilistic only)
  sched_yield();
  usleep(50000);
  sched_yield();
  EXPECT_EQ(resumeCounter, 2);

  // Re-Resume
  EXPECT_NO_THROW(p.resume());

  // Waiting for execution times to update
  pthread_barrier_wait(&barrier);
  EXPECT_EQ(resumeCounter, 3);

  // Terminate
  EXPECT_NO_THROW(p.terminate());
  EXPECT_THROW(p.initialize(), HiCR::RuntimeException);
  EXPECT_THROW(p.start(fc1), HiCR::RuntimeException);
  EXPECT_THROW(p.resume(), HiCR::RuntimeException);
  EXPECT_THROW(p.suspend(), HiCR::RuntimeException);
  EXPECT_THROW(p.terminate(), HiCR::RuntimeException);

  // Awaiting termination
  EXPECT_NO_THROW(p.await());
  EXPECT_THROW(p.start(fc1), HiCR::RuntimeException);
  EXPECT_THROW(p.resume(), HiCR::RuntimeException);
  EXPECT_THROW(p.suspend(), HiCR::RuntimeException);
  EXPECT_THROW(p.terminate(), HiCR::RuntimeException);

  ///////// Checking re-run same thread

  // Creating re-runner function
  auto fc2 = [&resumeCounter, &barrier, &suspendCounter]()
  {
    // Checking correct execution
    resumeCounter++;
    while(true) pthread_barrier_wait(&barrier);
  };

  // Reinitializing
  EXPECT_NO_THROW(p.initialize());

  // Re-running
  EXPECT_NO_THROW(p.start(fc2));

  // Waiting for resume counter to update
  pthread_barrier_wait(&barrier);
  EXPECT_EQ(resumeCounter, 4);

  // Re-terminating
  EXPECT_NO_THROW(p.terminate());

  // Re-awaiting
  EXPECT_NO_THROW(p.await());

  ///////////////// Creating case where the thread runs a function that finishes
  auto fc3 = []()
  {
  };

  // Reinitializing
  EXPECT_NO_THROW(p.initialize());

  // Re-running
  EXPECT_NO_THROW(p.start(fc3));

  // Re-terminating
  EXPECT_NO_THROW(p.terminate());

  // Re-awaiting
  EXPECT_NO_THROW(p.await());
}
