/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file thread.hpp
 * @brief Implements the Thread class for the pthreads-based backend.
 * @author S. M. Martin
 * @date 14/8/2023
 */

#pragma once

#include <csignal>
#include <fcntl.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <hicr/computeResource.hpp>

namespace HiCR
{

namespace backend
{

namespace sharedMemory
{

/**
 * Implementation of a kernel-level thread as HiCR computational resource.
 *
 * This implementation uses PThreads as backend for the creation and management of OS threads..
 */
class ProcessingUnit final : public ComputeResource
{
  private:

  /**
   * Stores the thread id as returned by the Pthreads library upon creation
   */
  pthread_t _pthreadId;

  /**
   * Stores the core affinity for the OS thread
   */
  std::vector<int> _affinity;

  /**
   * Static wrapper function to setup affinity and run the thread's function
   *
   * \param[in] p Pointer to a Thread class to recover the calling instance from inside wrapper
   */
  __USED__ inline static void *launchWrapper(void *p)
  {
    // Gathering thread object
    auto processingUnit = (ProcessingUnit *)p;

    // Setting initial thread affinity
    processingUnit->updateAffinity(processingUnit->_affinity);

    // Yielding execution to allow affinity to refresh
    sched_yield();

    // Calling main loop
    processingUnit->_fc();

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
    if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) != 0) printf("[WARNING] Problem assigning affinity: %d.\n", affinity[0]);
  }

  /**
   * Queries the OS for the currently set affinity for this thread, and prints it to screen.
   */
  __USED__ static void printAffinity()
  {
    cpu_set_t cpuset;
    if (pthread_getaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) != 0) printf("[WARNING] Problem obtaining affinity.\n");
    for (int i = 0; i < CPU_SETSIZE; i++)
      if (CPU_ISSET(i, &cpuset)) printf("%2d ", i);
  }

  /**
   * Handler for the SIGUSR1 signal, used by HiCR to suspend/resume worker threads
   *
   * \param[in] sig Signal detected, set by the operating system upon detecting the signal
   */
  __USED__ inline static void catchSIGUSR1Signal(int sig) { signal(sig, ProcessingUnit::catchSIGUSR1Signal); }

  public:

  /**
   * Constructor for the thread
   *
   * \param[in] affinity The affinity to set for the thread
   */
  ProcessingUnit(const std::vector<int> &affinity) : ComputeResource(), _affinity{affinity} {};

  __USED__ inline void initialize() override
  {
  }

  __USED__ inline void suspend() override
  {
    int status = 0;
    int signalSet;
    sigset_t suspendSet;

    signal(SIGUSR1, ProcessingUnit::catchSIGUSR1Signal);

    status = sigaddset(&suspendSet, SIGUSR1);
    if (status != 0) LOG_ERROR("Could not suspend thread %lu\n", _pthreadId);

    status = sigwait(&suspendSet, &signalSet);
    if (status != 0) LOG_ERROR("Could not suspend thread %lu\n", _pthreadId);
  }

  __USED__ inline void resume() override
  {
    auto status = pthread_kill(_pthreadId, SIGUSR1);
    if (status != 0) LOG_ERROR("Could not resume thread %lu\n", _pthreadId);
  }

  __USED__ inline void run(resourceFc_t fc) override
  {
    // Making a copy of the function
    _fc = fc;

    // Launching thread function wrapper
    auto status = pthread_create(&_pthreadId, NULL, launchWrapper, this);
    if (status != 0) LOG_ERROR("Could not create thread %lu\n", _pthreadId);
  }

  __USED__ inline void finalize() override
  {
  }

  __USED__ inline void await() override
  {
    // Waiting for thread after execution
    pthread_join(_pthreadId, NULL);
  }
};

} // end namespace sharedMemory

} // namespace backend

} // namespace HiCR
