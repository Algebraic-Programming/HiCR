/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file processingUnit.hpp
 * @brief Implements the processing unit class for the pthread-based host (CPU) backend.
 * @author S. M. Martin
 * @date 14/8/2023
 */

#pragma once

#include <csignal>
#include <set>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <pthread.h>
#include <hicr/exceptions.hpp>
#include <hicr/L0/processingUnit.hpp>
#include <backends/host/L0/executionState.hpp>
#include <backends/host/L0/executionUnit.hpp>
#include <backends/host/L0/computeResource.hpp>

namespace HiCR
{

namespace backend
{

namespace host
{

namespace pthreads
{

namespace L0
{

/**
 * Signal to use to suspend thread (might need to be adapted to each system)
 */
#define HICR_SUSPEND_RESUME_SIGNAL SIGUSR1

class ProcessingUnit;

/**
 * Implementation of a kernel-level thread as processing unit for the pthread-based host (CPU) backend.
 *
 * This implementation uses PThreads as backend for the creation and management of OS threads.
 */
class ProcessingUnit final : public HiCR::L0::ProcessingUnit
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
    cpu_set_t     cpuset;
    if (pthread_getaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) != 0) HICR_THROW_RUNTIME("Problem obtaining affinity.");
    for (int i = 0; i < CPU_SETSIZE; i++)
      if (CPU_ISSET(i, &cpuset)) affinity.insert(i);

    return affinity;
  }

  /**
   * Constructor for the ProcessingUnit class
   *
   * \param computeResource Represents the compute resource (core) affinity to associate this processing unit to
   */
  __USED__ inline ProcessingUnit(std::shared_ptr<HiCR::L0::ComputeResource> computeResource)
    : HiCR::L0::ProcessingUnit(computeResource)
  {
    // Getting up-casted pointer for the MPI instance
    auto c = dynamic_pointer_cast<HiCR::backend::host::L0::ComputeResource>(computeResource);

    // Checking whether the execution unit passed is compatible with this backend
    if (c == NULL) HICR_THROW_LOGIC("The passed compute resource is not supported by this processing unit type\n");
  };

  private:

  /**
   * Stores the thread id as returned by the Pthreads library upon creation
   */
  pthread_t _pthreadId;

  /**
   * Internal state of execution
   */
  std::unique_ptr<HiCR::L0::ExecutionState> _executionState;

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
    auto thread = (host::pthreads::L0::ProcessingUnit *)p;

    // Getting associated compute unit reference
    auto computeResource = dynamic_pointer_cast<HiCR::backend::host::L0::ComputeResource>(thread->getComputeResource());

    // Setting signal to hear for suspend/resume
    signal(HICR_SUSPEND_RESUME_SIGNAL, ProcessingUnit::catchSuspendResumeSignal);

    // Setting initial thread affinity
    thread->updateAffinity(std::set<int>({computeResource->getProcessorId()}));

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
    int      status = 0;
    int      signalSet;
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

  __USED__ inline void startImpl(std::unique_ptr<HiCR::L0::ExecutionState> executionState) override
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
  }

  __USED__ inline void awaitImpl() override
  {
    // Waiting for thread after execution
    pthread_join(_pthreadId, NULL);
  }
};

} // namespace L0

} // namespace pthreads

} // namespace host

} // namespace backend

} // namespace HiCR
