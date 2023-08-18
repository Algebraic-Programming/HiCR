// This is a minimal backend for multi-core support based on OpenMP

#pragma once

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
class Thread : public ComputeResource
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
  static void *launchWrapper(void *p)
  {
    // Gathering thread object
    auto thread = (Thread *)p;

    // Setting initial thread affinity
    thread->updateAffinity(thread->_affinity);

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
  static void updateAffinity(const std::vector<int> &affinity)
  {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    for (size_t i = 0; i < affinity.size(); i++) CPU_SET(affinity[i], &cpuset);
    if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) != 0) printf("[WARNING] Problem assigning affinity: %d.\n", affinity[0]);
  }

  /**
   * Queries the OS for the currently set affinity for this thread, and prints it to screen.
   */
  static void printAffinity()
  {
    cpu_set_t cpuset;
    if (pthread_getaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) != 0) printf("[WARNING] Problem obtaining affinity.\n");
    for (int i = 0; i < CPU_SETSIZE; i++)
      if (CPU_ISSET(i, &cpuset)) printf("%2d ", i);
  }

  public:
  /**
   * Constructor for the thread
   *
   * \param[in] affinity The affinity to set for the thread
   */
  Thread(const std::vector<int> &affinity) : ComputeResource(), _affinity{affinity} {};
  ~Thread() = default;

  void initialize() override
  {
  }

  void run(resourceFc_t fc) override
  {
    // Making a copy of the function
    _fc = fc;

    // Launching thread function wrapper
    auto status = pthread_create(&_pthreadId, NULL, launchWrapper, this);
    if (status != 0) LOG_ERROR("Could not create thread %lu\n", _pthreadId);
  }

  void finalize() override
  {
  }

  void await() override
  {
    // Waiting for thread after execution
    pthread_join(_pthreadId, NULL);
  }
};

} // namespace sharedMemory

} // namespace backend

} // namespace HiCR
