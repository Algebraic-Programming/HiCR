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

#include <csignal>
#include <fcntl.h>
#include <hicr/common/exceptions.hpp>
#include <hicr/processingUnit.hpp>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/stat.h>

namespace HiCR
{

namespace backend
{

namespace sharedMemory
{

/**
 * Implementation of a kernel-level thread as processing unit for the shared memory backend.
 *
 * This implementation uses PThreads as backend for the creation and management of OS threads..
 */
class Thread final : public ProcessingUnit
{
  private:

  /**
   * Stores the thread id as returned by the Pthreads library upon creation
   */
  pthread_t _pthreadId;

  /**
   * Copy of the function to be ran by the thread
   */
  processingUnitFc_t _fc;

  /**
   * Static wrapper function to setup affinity and run the thread's function
   *
   * \param[in] p Pointer to a Thread class to recover the calling instance from inside wrapper
   */
  __USED__ inline static void *launchWrapper(void *p)
  {
    // Gathering thread object
    auto thread = (Thread *)p;

    // Setting initial thread affinity
    thread->updateAffinity(std::vector<int>({(int)thread->getComputeResourceId()}));

    // Yielding execution to allow affinity to refresh
    sched_yield();

    // Calling main loop
    thread->_fc();

    // No returns
    return NULL;
  }

  /**
   * Sets up new affinity for the thread. The thread needs to yield or be preempted for the new affinity to work.
   *
   * \param[in] affinity New affinity to use
   */
  __USED__ static void updateAffinity(const std::vector<int> &affinity)
  {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    for (size_t i = 0; i < affinity.size(); i++) CPU_SET(affinity[i], &cpuset);
    if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) != 0) HICR_THROW_RUNTIME("Problem assigning affinity: %d.", affinity[0]);
  }

  /**
   * Queries the OS for the currently set affinity for this thread, and prints it to screen.
   */
  __USED__ static void printAffinity()
  {
    cpu_set_t cpuset;
    if (pthread_getaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) != 0) HICR_THROW_RUNTIME("[WARNING] Problem obtaining affinity.");
    for (int i = 0; i < CPU_SETSIZE; i++)
      if (CPU_ISSET(i, &cpuset)) printf("%2d ", i);
  }

  /**
   * Handler for the SIGUSR1 signal, used by HiCR to suspend/resume worker threads
   *
   * \param[in] sig Signal detected, set by the operating system upon detecting the signal
   */
  __USED__ inline static void catchSIGUSR1Signal(int sig) { signal(sig, Thread::catchSIGUSR1Signal); }

  __USED__ inline void initializeImpl() override
  {
  }

  __USED__ inline void suspendImpl() override
  {
    int status = 0;
    int signalSet;
    sigset_t suspendSet;

    signal(SIGUSR1, Thread::catchSIGUSR1Signal);

    status = sigaddset(&suspendSet, SIGUSR1);
    if (status != 0) HICR_THROW_RUNTIME("Could not suspend thread %lu\n", _pthreadId);

    status = sigwait(&suspendSet, &signalSet);
    if (status != 0) HICR_THROW_RUNTIME("Could not suspend thread %lu\n", _pthreadId);
  }

  __USED__ inline void resumeImpl() override
  {
    auto status = pthread_kill(_pthreadId, SIGUSR1);
    if (status != 0) HICR_THROW_RUNTIME("Could not resume thread %lu\n", _pthreadId);
  }

  __USED__ inline void startImpl(processingUnitFc_t fc) override
  {
    // Making a copy of the function
    _fc = fc;

    // Launching thread function wrapper
    auto status = pthread_create(&_pthreadId, NULL, launchWrapper, this);
    if (status != 0) HICR_THROW_RUNTIME("Could not create thread %lu\n", _pthreadId);
  }

  __USED__ inline void terminateImpl() override
  {
  }

  __USED__ inline void awaitImpl() override
  {
    // Waiting for thread after execution
    pthread_join(_pthreadId, NULL);
  }

  public:

  /**
   * Constructor for the Thread class
   *
   * \param core Represents the core affinity to associate this processing unit to
   */
  __USED__ inline Thread(computeResourceId_t core) : ProcessingUnit(core){};
};

} // end namespace sharedMemory

} // namespace backend

} // namespace HiCR
