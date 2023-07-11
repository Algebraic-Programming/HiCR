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

 bool _finalize = false;
 pthread_t _pthreadId;
 std::vector<int> _affinity;
 thread_state _state = thread_state::initial;

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
   for (auto pool : _pools)
   {
    auto task = pool->getNextTask();

    // If this pool contains no tasks, then go on to the next one
    if (task == NULL) continue;

    // Running task
    task->run();

    // If the task is marked as terminal, the current thread has to finish.
    if (task->isTerminal() == true)
    {
     _finalize = true;

     // We stop processing other tasks
     break;
    }
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

