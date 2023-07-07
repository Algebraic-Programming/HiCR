#pragma once
#include <taskr/runtime.hpp>
#include <taskr/task.hpp>
#include <taskr/worker.hpp>
#include <omp.h>

namespace taskr
{

// Boolean indicating whether the runtime system was initialized
inline bool _runtimeInitialized = false;

inline void addTask(Task* task)
{
 if (_runtimeInitialized == false) LOG_ERROR("Attempting to use Taskr without first initializing it.");

 // Increasing task count
 _runtime->_taskCount++;

 // Checking if maximum was exceeded
 if (_runtime->_taskCount >= MAX_SIMULTANEOUS_TASKS) LOG_ERROR("Maximum task size (MAX_SIMULTANEOUS_TASKS = %lu) exceeded.\n", MAX_SIMULTANEOUS_TASKS);

 // Adding task to the waiting list. Their dependencies will be checked later
 _runtime->_waitingTaskQueue.push(task);
}

inline void initialize()
{
 // Instantiating singleton
 _runtime = new Runtime();

 // Initializing HiCR runtime
 _runtime->_hicr.initialize();

 // Set initialized state
 _runtimeInitialized = true;
}

inline void run()
{
 if (_runtimeInitialized == false) LOG_ERROR("Attempting to use Taskr without first initializing it.");

 // Clearing runtime's thread id to worker map
 _runtime->_workerIdToWorkerMap.clear();

 // Querying HiCR for available workers
 auto backends = _runtime->_hicr.getBackends();

 // Gathering all resources that can execute worker tasks
 std::vector<HiCR::Resource*> resources;
 for (size_t i = 0; i < backends.size(); i++)
 {
  backends[i]->queryResources();
  const auto& backendResources = backends[i]->getResourceList();
  resources.insert(resources.end(), backendResources.begin(), backendResources.end());
 }

 // Instantiating workers
 #pragma omp parallel
 {
  // Creating worker
  Worker w;

  // Adding worker to map
  #pragma omp critical
  _runtime->_workerIdToWorkerMap[w.getWorkerId()] = &w;

  // Barrier to prevent accesing the worker map when others are still modifying it
  #pragma omp barrier

  // Running worker until all tasks are performed
  w.run();
 }
}

inline void finalize()
{
 if (_runtimeInitialized == false) LOG_ERROR("Attempting to use Taskr without first initializing it.");

 // Freeing up singleton
 delete _runtime;

 // Setting runtime to non-initialized
 _runtimeInitialized = false;
}

inline Worker* getWorker() { return _runtime->_workerIdToWorkerMap[pthread_self()]; }
inline Task* getTask() { return getWorker()->getCurrentTask(); }

} // namespace taskr

