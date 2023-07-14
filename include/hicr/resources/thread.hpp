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

class Thread : public Resource
{
 private:

 bool _finalize = false;
 pthread_t _pthreadId;
 std::vector<int> _affinity;

 static void* launchWrapper(void* p)
 {
  // Gathering thread object
  auto thread = (Thread*)p;

  // Setting initial thread affinity
  thread->updateAffinity(thread->_affinity);

  // Yielding execution to allow affinity to refresh
  sched_yield();

  // Calling main loop
  thread->mainLoop();

  // No returns
  return NULL;
 }

 static void updateAffinity(const std::vector<int>& affinity)
 {
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  for (size_t i = 0; i < affinity.size(); i++) CPU_SET(affinity[i], &cpuset);
  if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) != 0) printf("[WARNING] Problem assigning affinity: %d.\n", affinity[0]);
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
  while(_finalize == false)
  {
   for (auto dispatcher : _dispatchers)
   {
    // Attempt to both pop and pull from dispatcher
    auto task = dispatcher->pull();

    // If a task was returned, then execute it
    if (task != NULL) task->run();
   }
  }
 }

 public:

 Thread(const resourceId_t id, const std::vector<int>& affinity) : Resource(id),_affinity {affinity} { };
 ~Thread() = default;

 void await()
 {
  pthread_join(_pthreadId, NULL);
 }

 void finalize()
 {
  _finalize = true;
 }

 void initialize()
 {
  auto status = pthread_create(&_pthreadId, NULL, launchWrapper, this);
  if (status != 0) LOG_ERROR("Could not create thread %lu\n", _id);
 }

};

} // namespace pthreads

} // namespace backends

} // namespace HiCR

