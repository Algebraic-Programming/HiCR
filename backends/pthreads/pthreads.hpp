// This is a minimal backend for multi-core support based on OpenMP

#pragma once

#include "hwloc.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include <hicr/backend.hpp>
#include <../backends/pthreads/thread.hpp>


namespace HiCR {

namespace backends {

class PThreads : public Backend
{
 private:

 // Local processor and memory hierarchy topology, as detected by Hwloc
 hwloc_topology_t _topology;

 // Thread set
 std::vector<pthreads::Thread*> _threads;

 public:

 PThreads() = default;
 ~PThreads() = default;

 static void getThreadPUs (hwloc_topology_t topology, hwloc_obj_t obj, int depth, std::vector<int>& threadPUs)
 {
   if (obj->arity == 0) threadPUs.push_back(obj->os_index);
   for (unsigned int i = 0; i < obj->arity; i++) getThreadPUs(topology, obj->children[i], depth + 1, threadPUs);
 };

 void queryResources() override
 {

  hwloc_topology_init(&_topology);
  hwloc_topology_load(_topology);

  std::vector<int> threadPUs;
  getThreadPUs(_topology, hwloc_get_root_obj(_topology), 0, threadPUs);

  // Craeting Thread objects
  for (size_t i = 0; i < threadPUs.size(); i++)
  {
   auto affinity = std::vector<int>({threadPUs[i]});
   auto thread = new pthreads::Thread(i, affinity);
   _threads.push_back(thread);
   printf("Thread %lu assigned PU: %d\n", i, threadPUs[i]);
  }

  // Launching threads
  for (size_t i = 0; i < _threads.size(); i++) _threads[i]->launch();

  // Waiting for threads to finish
  for (size_t i = 0; i < _threads.size(); i++) pthread_join(_threads[i]->getPthreadId(), NULL);

  exit(0);

  // Creating resource list, one per openMP thread
//  _resourceList.clear();
//  for (int i = 0; i < threadCount; i++)
//   _resourceList.push_back(new openMP::Thread(i));
 }

};


} // namespace backends

} // namespace HiCR


