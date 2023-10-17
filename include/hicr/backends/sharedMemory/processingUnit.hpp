/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file thread.hpp
 * @brief Implements the processing unit class for the shared memory backend.
 * @author S. M. Martin
 * @date 14/8/2023
 */

#pragma once

#include <fcntl.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <csignal>
#include <set>
#include <hicr/common/exceptions.hpp>
#include <hicr/processingUnit.hpp>
#include <hicr/backends/sequential/executionUnit.hpp>
#include <hicr/backends/sequential/executionState.hpp>

namespace HiCR
{

namespace backend
{

namespace sharedMemory
{

/**
 * Signal to use to suspend thread (might need to be adapted to each system)
 */
#define HICR_SUSPEND_RESUME_SIGNAL SIGUSR1

/**
 * Implementation of a kernel-level thread as processing unit for the shared memory backend.
 *
 * This implementation uses PThreads as backend for the creation and management of OS threads..
 */
class ProcessingUnit final : public HiCR::ProcessingUnit
{
  public:

  /**
   * Sets up new affinity for the thread. The thread needs to yield or be preempted for the new affinity to work.
   *
   * \param[in] affinity New affinity to use
   */
  __USED__ static void updateAffinity(const std::set<int> &affinity)
  {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    for (const auto c : affinity) CPU_SET(c, &cpuset);
    if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) != 0) HICR_THROW_RUNTIME("Problem assigning affinity.");
  }

  /**
   * Queries the OS for the currently set affinity for this thread, and prints it to screen.
   *
   * \return The set of cores/processing units that this thread is bound to
   */
  __USED__ static std::set<int> getAffinity()
  {
    std::set<int> affinity;
    cpu_set_t cpuset;
    if (pthread_getaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) != 0) HICR_THROW_RUNTIME("Problem obtaining affinity.");
    for (int i = 0; i < CPU_SETSIZE; i++)
      if (CPU_ISSET(i, &cpuset)) affinity.insert(i);

    return affinity;
  }

  /**
   * Constructor for the ProcessingUnit class
   *
   * \param core Represents the core affinity to associate this processing unit to
   */
  __USED__ inline ProcessingUnit(computeResourceId_t core) : HiCR::ProcessingUnit(core){};

  private:

  /**
   * Stores the thread id as returned by the Pthreads library upon creation
   */
  pthread_t _pthreadId;

  /**
   * Internal state of execution
   */
  std::unique_ptr<ExecutionState> _executionState;

  /**
   * Barrier to synchronize thread initialization
   */
  pthread_barrier_t initializationBarrier;

  /**
   * Static wrapper function to setup affinity and run the thread's function
   *
   * \param[in] p Pointer to a ProcessingUnit class to recover the calling instance from inside wrapper
   */
  __USED__ inline static void *launchWrapper(void *p)
  {
    // Gathering thread object
    auto thread = (ProcessingUnit *)p;

    // Setting signal to hear for suspend/resume
    signal(HICR_SUSPEND_RESUME_SIGNAL, ProcessingUnit::catchSuspendResumeSignal);

    // Setting initial thread affinity
    thread->updateAffinity(std::set<int>({(int)thread->getComputeResourceId()}));

    // Yielding execution to allow affinity to refresh
    sched_yield();

    // The thread has now been properly initialized
    pthread_barrier_wait(&thread->initializationBarrier);

    // Calling main loop
    thread->_executionState->resume();

    // No returns
    return NULL;
  }

  /**
   * Handler for the suspend/resume signal, used by HiCR to suspend/resume worker threads
   *
   * \param[in] sig Signal detected, set by the operating system upon detecting the signal
   */
  __USED__ inline static void catchSuspendResumeSignal(int sig)
  {
    int status = 0;
    int signalSet;
    sigset_t suspendSet;

    // Waiting for that signal to arrive
    status = sigaddset(&suspendSet, HICR_SUSPEND_RESUME_SIGNAL);
    if (status != 0) HICR_THROW_RUNTIME("Could not suspend thread\n");

    status = sigwait(&suspendSet, &signalSet);
    if (status != 0) HICR_THROW_RUNTIME("Could not suspend thread\n");

    // Re-set next signal listening before exiting
    signal(HICR_SUSPEND_RESUME_SIGNAL, ProcessingUnit::catchSuspendResumeSignal);
  }

  __USED__ inline void initializeImpl() override
  {
  }

  __USED__ inline void suspendImpl() override
  {
    auto status = pthread_kill(_pthreadId, HICR_SUSPEND_RESUME_SIGNAL);
    if (status != 0) HICR_THROW_RUNTIME("Could not suspend thread %lu\n", _pthreadId);
  }

  __USED__ inline void resumeImpl() override
  {
    auto status = pthread_kill(_pthreadId, HICR_SUSPEND_RESUME_SIGNAL);
    if (status != 0) HICR_THROW_RUNTIME("Could not resume thread %lu\n", _pthreadId);
  }

  __USED__ inline void startImpl(std::unique_ptr<HiCR::ExecutionState> executionState) override
  {
   // Initializing barrier
   pthread_barrier_init(&initializationBarrier, NULL, 2);

   // Obtaining execution state
  _executionState = std::move(executionState);

   // Launching thread function wrapper
   auto status = pthread_create(&_pthreadId, NULL, launchWrapper, this);
   if (status != 0) HICR_THROW_RUNTIME("Could not create thread %lu\n", _pthreadId);

   // Waiting for proper initialization of the thread
   pthread_barrier_wait(&initializationBarrier);

   // Destroying barrier
   pthread_barrier_destroy(&initializationBarrier);
  }

  __USED__ inline void terminateImpl() override
  {
    // Yielding current execution state
    _executionState->yield();
  }

  __USED__ inline void awaitImpl() override
  {
    // Waiting for thread after execution
    pthread_join(_pthreadId, NULL);
  }

  __USED__ inline std::unique_ptr<HiCR::ExecutionState> createExecutionState(const HiCR::ExecutionUnit* executionUnit) override
  {
   // Getting up-casted pointer for the execution unit
   auto e = dynamic_cast<const sequential::ExecutionUnit*>(executionUnit);

   // Checking whether the execution unit passed is compatible with this backend
   if (e == NULL) HICR_THROW_FATAL("The passed execution of type '%s' is not supported by this backend\n", executionUnit->getType());

   // Creating and returning new execution state
   return std::make_unique<sequential::ExecutionState>(e);
  }
};

} // end namespace sharedMemory

} // namespace backend

} // namespace HiCR
