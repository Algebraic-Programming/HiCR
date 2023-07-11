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

 // Creating individual task pools for the TaskR worker tasks
 std::vector<HiCR::TaskPool*> taskPools;
 for (size_t i = 0; i < resources.size(); i++)
 {
  auto taskPool = new HiCR::TaskPool();
  resources[i]->subscribe(taskPool);
  taskPools.push_back(taskPool);
 }

 // Creating workers and dispatching their tasks tasks
 std::vector<Worker*> workers;
 std::vector<HiCR::Task*> workerTasks;
 for (size_t i = 0; i < taskPools.size(); i++)
 {
  auto worker = new Worker();
  auto workerTask = new HiCR::Task([](void* worker){((Worker*)worker)->run();});
  taskPools[i]->dispatchTask(workerTask, worker);
 }

 // Initializing resources
 for (size_t i = 0; i < resources.size(); i++) resources[i]->initialize();

 printf("Back on TaskR\n");
 fflush(stdout);
 sleep(2);
 exit(0);
}

inline void finalize()
{
 if (_runtimeInitialized == false) LOG_ERROR("Attempting to use Taskr without first initializing it.");

 // Freeing up singleton
 delete _runtime;

 // Setting runtime to non-initialized
 _runtimeInitialized = false;
}

} // namespace taskr

