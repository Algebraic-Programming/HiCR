/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file processingUnit.hpp
 * @brief Implements the processing unit class for the Pthreads backend.
 * @author S. M. Martin
 * @date 14/8/2023
 */

#pragma once

#include <csignal>
#include <set>
#include <sched.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <pthread.h>
#include <hicr/core/definitions.hpp>
#include <hicr/core/exceptions.hpp>
#include <hicr/core/L0/processingUnit.hpp>
#include <hicr/backends/boost/L0/executionState.hpp>
#include <hicr/backends/boost/L0/executionUnit.hpp>
#include <hicr/backends/hwloc/L0/computeResource.hpp>

#ifndef _GNU_SOURCE
  /// Required by sched_getaffinity
  #define _GNU_SOURCE
#endif

namespace HiCR::backend::pthreads::L1
{
class ComputeManager;
}

namespace HiCR::backend::pthreads::L0
{

/**
 * Signal to use to suspend thread (might need to be adapted to each system)
 */
#define HICR_SUSPEND_SIGNAL SIGUSR1

/**
 * Signal to use to resume thread (might need to be adapted to each system)
 */
#define HICR_RESUME_SIGNAL SIGUSR2

class ProcessingUnit;

/**
 * Implementation of a kernel-level thread as processing unit for the pthreads backend.
 *
 * This implementation uses Pthreads as backend for the creation and management of OS threads.
 */
class ProcessingUnit final : public HiCR::L0::ProcessingUnit
{
  friend class HiCR::backend::pthreads::L1::ComputeManager;

  public:

  [[nodiscard]] __INLINE__ std::string getType() override { return "POSIX Thread"; }

  /**
   * Sets up new affinity for the thread. The thread needs to yield or be preempted for the new affinity to work.
   *
   * \param[in] affinity New affinity to use
   */
  __INLINE__ static void updateAffinity(const std::set<hwloc::L0::ComputeResource::logicalProcessorId_t> &affinity)
  {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    for (const auto c : affinity) CPU_SET_S(c, sizeof(cpu_set_t), &cpuset);

    // Attempting to use the pthread interface first
    int status = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);

    // If failed, attempt to use the sched interface
    if (status != 0) status = sched_getaffinity(getpid(), sizeof(cpu_set_t), &cpuset);

    // Throw exception if none of them worked
    if (status != 0) HICR_THROW_RUNTIME("Problem assigning affinity.");
  }

  /**
   * Queries the OS for the currently set affinity for this thread, and prints it to screen.
   *
   * \return The set of cores/processing units that this thread is bound to
   */
  __INLINE__ static std::set<HiCR::backend::hwloc::L0::ComputeResource::logicalProcessorId_t> getAffinity()
  {
    auto      affinity = std::set<HiCR::backend::hwloc::L0::ComputeResource::logicalProcessorId_t>();
    cpu_set_t cpuset;

    // Attempting to use the pthread interface first
    int status = pthread_getaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);

    // If failed, attempt to use the sched interface
    if (status != 0) status = sched_getaffinity(getpid(), sizeof(cpu_set_t), &cpuset);

    // Throw exception if none of them worked
    if (status != 0) HICR_THROW_RUNTIME("Problem obtaining affinity.");
    for (int i = 0; i < CPU_SETSIZE; i++)
      if (CPU_ISSET(i, &cpuset)) affinity.insert(i);

    return affinity;
  }

  /**
   * Constructor for the ProcessingUnit class
   *
   * \param computeResource Represents the compute resource (core) affinity to associate this processing unit to
   */
  __INLINE__ ProcessingUnit(const std::shared_ptr<HiCR::L0::ComputeResource> &computeResource)
    : HiCR::L0::ProcessingUnit(computeResource)
  {
    // Getting up-casted pointer for the processing unit
    auto c = dynamic_pointer_cast<HiCR::backend::hwloc::L0::ComputeResource>(computeResource);

    // Checking whether the execution unit passed is compatible with this backend
    if (c == nullptr) HICR_THROW_LOGIC("The passed compute resource is not supported by this processing unit type\n");

    // Creating initialization barrier
    _initializationBarrier = std::make_unique<pthread_barrier_t>();
  };

  private:

  /**
   * Stores the thread id as returned by the Pthreads library upon creation
   */
  pthread_t _pthreadId = 0;

  /**
   * Internal state of execution
   */
  std::unique_ptr<HiCR::L0::ExecutionState> _executionState;

  /**
   * Barrier to synchronize thread initialization
   */
  std::unique_ptr<pthread_barrier_t> _initializationBarrier;

  /**
   * Static wrapper function to setup affinity and run the thread's function
   *
   * \param[in] p Pointer to a ProcessingUnit class to recover the calling instance from inside wrapper
   */
  __INLINE__ static void *launchWrapper(void *p)
  {
    // Gathering thread object
    auto thread = static_cast<pthreads::L0::ProcessingUnit *>(p);

    // Getting associated compute unit reference
    auto computeResource = dynamic_pointer_cast<HiCR::backend::hwloc::L0::ComputeResource>(thread->getComputeResource());

    // Setting signal to hear for suspend/resume
    signal(HICR_SUSPEND_SIGNAL, ProcessingUnit::catchSuspendSignal);
    signal(HICR_RESUME_SIGNAL, ProcessingUnit::catchResumeSignal);

    // Setting initial thread affinity
    thread->updateAffinity(std::set<hwloc::L0::ComputeResource::logicalProcessorId_t>({computeResource->getProcessorId()}));

    // Yielding execution to allow affinity to refresh
    sched_yield();

    // The thread has now been properly initialized
    pthread_barrier_wait(thread->_initializationBarrier.get());

    // Calling main loop
    thread->_executionState->resume();

    // No returns
    return nullptr;
  }

  /**
   * Handler for the suspend signal, used by HiCR to suspend worker threads
   *
   * \param[in] sig Signal detected, set by the operating system upon detecting the signal
   */
  __INLINE__ static void catchSuspendSignal(int sig)
  {
    int      status    = 0;
    int      signalSet = 0;
    sigset_t suspendSet;

    // Waiting for that signal to arrive
    status = sigaddset(&suspendSet, HICR_RESUME_SIGNAL);
    if (status != 0) HICR_THROW_RUNTIME("Could not set resume signal thread\n");

    status = sigwait(&suspendSet, &signalSet);
    if (status != 0) HICR_THROW_RUNTIME("Could not suspend thread\n");
  }

  /**
   * Handler for the resume signal, used by HiCR to resume worker threads
   *
   * \param[in] sig Signal detected, set by the operating system upon detecting the signal
   */
  __INLINE__ static void catchResumeSignal(int sig) {}

  __INLINE__ void initialize()
  {
    // Nothing to do for the initialization
  }

  __INLINE__ void suspend()
  {
    auto status = pthread_kill(_pthreadId, HICR_SUSPEND_SIGNAL);
    if (status != 0) HICR_THROW_RUNTIME("Could not suspend thread %lu\n", _pthreadId);
  }

  __INLINE__ void resume()
  {
    auto status = pthread_kill(_pthreadId, HICR_RESUME_SIGNAL);
    if (status != 0) HICR_THROW_RUNTIME("Could not resume thread %lu\n", _pthreadId);
  }

  __INLINE__ void start(std::unique_ptr<HiCR::L0::ExecutionState> &executionState)
  {
    // Initializing barrier
    pthread_barrier_init(_initializationBarrier.get(), nullptr, 2);

    // Obtaining execution state
    _executionState = std::move(executionState);

    // Launching thread function wrapper
    auto status = pthread_create(&_pthreadId, nullptr, launchWrapper, this);
    if (status != 0) HICR_THROW_RUNTIME("Could not create thread %lu\n", _pthreadId);

    // Waiting for proper initialization of the thread
    pthread_barrier_wait(_initializationBarrier.get());

    // Destroying barrier
    pthread_barrier_destroy(_initializationBarrier.get());
  }

  __INLINE__ void terminate()
  {
    // Nothing to do actively, just wait for the thread to finalize in its own accord
  }

  __INLINE__ void await()
  {
    // Waiting for thread after execution
    pthread_join(_pthreadId, nullptr);
  }
};

} // namespace HiCR::backend::pthreads::L0
