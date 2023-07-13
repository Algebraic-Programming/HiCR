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

static inline void onTaskFinish(HiCR::Task* task)
{
  _runtime->_workerCount--;
  _runtime->taskToResourceMap[task]->finalize();
}

inline void run()
{
 if (_runtimeInitialized == false) LOG_ERROR("Attempting to use Taskr without first initializing it.");

 // Querying HiCR for available workers
 _runtime->backends = _runtime->_hicr.getBackends();

 // Gathering all resources that can execute worker tasks
 for (size_t i = 0; i < _runtime->backends.size(); i++)
 {
  _runtime->backends[i]->queryResources();
  const auto& backendResources = _runtime->backends[i]->getResourceList();
  _runtime->resources.insert(_runtime->resources.end(), backendResources.begin(), backendResources.end());
 }

 // Creating event map ands events
 _runtime->eventMap.setEvent(HiCR::event_t::onTaskFinish, [](HiCR::Task* task){onTaskFinish(task);});

 // Creating workers and their tasks
 for (size_t i = 0; i < _runtime->resources.size(); i++)
 {
  auto dispatcher = new HiCR::Dispatcher();
  auto worker = new HiCR::Task([](void* arg){Worker::run();});
  worker->setEventMap(&_runtime->eventMap);
  dispatcher->push(worker);
  _runtime->resources[i]->subscribe(dispatcher);
  _runtime->taskToResourceMap[worker] = _runtime->resources[i];
  _runtime->workers.push_back(worker);
  _runtime->dispatchers.push_back(dispatcher);
 }

 // Set initial worker count
 _runtime->_workerCount = _runtime->workers.size();

 // Initializing workers
 for (size_t i = 0; i < _runtime->resources.size(); i++) _runtime->resources[i]->initialize();

 // Waiting for workers to finish (i.e., all tasks have finished)
 for (size_t i = 0; i < _runtime->resources.size(); i++) _runtime->resources[i]->await();

 // Clearing resources
 for (auto& x : _runtime->backends) delete x;
 for (auto& x : _runtime->resources) delete x;
 for (auto& x : _runtime->dispatchers) delete x;
 for (auto& x : _runtime->workers) delete x;
 _runtime->backends.clear();
 _runtime->resources.clear();
 _runtime->dispatchers.clear();
 _runtime->workers.clear();

 _runtime->taskToResourceMap.clear();
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

