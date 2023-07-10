// This is a minimal backend for multi-core support based on OpenMP

#pragma once

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <hicr/resource.hpp>
#include <pthread.h>

namespace HiCR {

namespace backends {

namespace pthreads {

enum thread_state { initial, running, finished };


class Thread : public Resource
{
 private:

 pthread_t _pthreadId;
 std::vector<int> _affinity;
 thread_state _state = thread_state::initial;

 static void* launchWrapper(void* p)
 {
  // Gathering thread object
  auto thread = (Thread*)p;

  // Setting initial thread affinity
  thread->updateAffinity();

  // Yielding execution to allow affinity to refresh
  sched_yield();

  // Calling main loop
  thread->mainLoop();

  // No returns
  return NULL;
 }

 void updateAffinity() const
 {
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  for (size_t i = 0; i < _affinity.size(); i++) CPU_SET(_affinity[i], &cpuset);
  if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) != 0) printf("[WARNING] Problem assigning affinity: %d.\n", _affinity[0]);
 }

 static void printAffinity()
 {
   cpu_set_t cpuset;
   if (pthread_getaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) != 0)  printf("[WARNING] Problem obtaining affinity.\n");
    for (int i = 0; i < CPU_SETSIZE; i++)
    if (CPU_ISSET(i, &cpuset)) printf("%2d ", i);
 }


 void mainLoop()
 {
  printAffinity();

  while(1);
 }

 public:

 Thread(const resourceId_t id, const std::vector<int>& affinity) : Resource(id),_affinity {affinity} { };
 ~Thread() = default;

 void launch()
 {
  auto status = pthread_create(&_pthreadId, NULL, launchWrapper, this);
  if (status != 0) LOG_ERROR("Could not create thread %lu\n", _id);
 }

 pthread_t getPthreadId() const
 {
  return _pthreadId;
 }

};

} // namespace pthreads

} // namespace backends

} // namespace HiCR

