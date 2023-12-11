/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file runtime.hpp
 * @brief This file implements the main TaskR Runtime class
 * @author Sergio Martin
 * @date 8/8/2023
 */

#pragma once
#include <atomic>
#include <frontends/taskr/common.hpp>
#include <frontends/taskr/task.hpp>
#include <hicr/L1/tasking/task.hpp>
#include <hicr/L1/tasking/worker.hpp>
#include <map>
#include <mutex>

namespace taskr
{

/**
 * Implementation of the TaskR Runtime singleton class
 *
 * It holds the entire running state of the tasks and the dependency graph.
 */
class Runtime
{
  private:

  /**
   * Pointer to the internal HiCR event map, required to capture finishing or yielding tasks
   */
  HiCR::L1::tasking::Task::taskEventMap_t *_eventMap;

  /**
   * Single dispatcher that distributes pending tasks to idle workers as they become idle
   */
  HiCR::L1::tasking::Dispatcher *_dispatcher;

  /**
   * Set of workers assigned to execute tasks
   */
  std::vector<HiCR::L1::tasking::Worker *> _workers;

  /**
   * Stores the current number of active tasks. This is an atomic counter that, upon reaching zero,
   * indicates that no more work remains to be done and the runtime system may return execution to the user.
   */
  std::atomic<uint64_t> _taskCount;

  /**
   * Lock-free queue for waiting tasks.
   */
  HiCR::lockFreeQueue_t<Task *, MAX_SIMULTANEOUS_TASKS> _waitingTaskQueue;

  /**
   * Hash map for quick location of tasks based on their hashed names
   */
  HiCR::parallelHashSet_t<taskLabel_t> _finishedTaskHashMap;

  /**
   * Mutex for the active worker queue, required for the max active workers mechanism
   */
  std::mutex _activeWorkerQueueLock;

  /**
   * User-defined maximum active worker count. Required for the max active workers mechanism
   * A zero value indicates that there is no limitation to the maximum active worker count.
   */
  size_t _maximumActiveWorkers = 0;

  /**
   * Keeps track of the currently active worker count. Required for the max active workers mechanism
   */
  ssize_t _activeWorkerCount;

  /**
   * Lock-free queue storing workers that remain in suspension. Required for the max active workers mechanism
   */
  HiCR::lockFreeQueue_t<HiCR::L1::tasking::Worker *, MAX_SIMULTANEOUS_WORKERS> _suspendedWorkerQueue;

  /**
   * The processing units assigned to taskr to run workers from
   */
  std::vector<std::unique_ptr<HiCR::L0::ProcessingUnit>> _processingUnits;

  /**
   * This function checks whether a given task is ready to go (i.e., all its dependencies have been satisfied)
   *
   * \param[in] task The task to check
   * \return true, if the task is ready; false, if the task is not ready
   */
  __USED__ inline bool checkTaskReady(Task *task)
  {
    const auto &dependencies = task->getDependencies();

    // Checking Task dependencies
    for (size_t i = 0; i < dependencies.size(); i++)
      if (_finishedTaskHashMap.contains(dependencies[i]) == false)
        return false; // If any unsatisfied dependency was found, return
                      // immediately

    return true;
  }

  /**
   * This function implements the auto-sleep mechanism that limits the number of active workers based on a user configuration
   * It will put any calling worker to sleep if the number of active workers exceed the maximum.
   * If the number of active workers is smaller than the maximum, it will try to 'wake up' other suspended workers, if any,
   * until the maximum is reached again.
   */
  __USED__ inline void checkMaximumActiveWorkerCount()
  {
    // Getting a pointer to the currently executing worker
    auto worker = HiCR::L1::tasking::Worker::getCurrentWorker();

    // Try to get the active worker queue lock, otherwise keep going
    if (_activeWorkerQueueLock.try_lock())
    {
      // If the number of workers exceeds that of maximum active workers,
      // suspend current worker
      if (_maximumActiveWorkers > 0 &&
          _activeWorkerCount > (ssize_t)_maximumActiveWorkers)
      {
        // Adding worker to the queue
        _suspendedWorkerQueue.push(worker);

        // Reducing active worker count
        _activeWorkerCount--;

        // Releasing lock, this is necessary here because the worker is going
        // into sleep
        _activeWorkerQueueLock.unlock();

        // Suspending worker
        worker->suspend();

        // Returning now, because we've already released the lock and shouldn't
        // do anything else without it
        return;
      }

      // If the new maximum is higher than the number of active workers, we need
      // to re-awaken some of them
      while ((_maximumActiveWorkers == 0 ||
              (ssize_t)_maximumActiveWorkers > _activeWorkerCount) &&
             _suspendedWorkerQueue.was_size() > 0)
      {
        // Pointer to the worker to wake up
        HiCR::L1::tasking::Worker *w = NULL;

        // Getting the worker from the queue of suspended workers
        _suspendedWorkerQueue.try_pop(w);

        // Increase the active worker count
        _activeWorkerCount++;

        // Resuming worker
        w->resume();
      }

      // Releasing lock
      _activeWorkerQueueLock.unlock();
    }
  }

  public:

  /**
   * Constructor of the TaskR Runtime.
   */
  Runtime() = default;

  /**
   * This function adds a processing unit to be used by TaskR in the execution of tasks
   *
   * \param[in] pu The processing unit to add
   */
  __USED__ inline void addProcessingUnit(std::unique_ptr<HiCR::L0::ProcessingUnit> pu)
  {
    _processingUnits.push_back(std::move(pu));
  }

  /**
   * Sets the maximum active worker count. If the current number of active workers exceeds this maximu, TaskR will put as many
   * workers to sleep as necessary to get the active count as close as this value as possible. If this maximum is later increased,
   * then any suspended workers will be awaken by active workers.
   *
   * \param[in] max The desired number of maximum workers. A non-positive value means that there is no limit.
   */
  __USED__ inline void setMaximumActiveWorkers(const size_t max)
  {
    // Storing new maximum active worker count
    _maximumActiveWorkers = max;
  }

  /**
   * Adds a task to the TaskR runtime for execution. This can be called at any point, before or during the execution of TaskR.
   *
   * \param[in] task Task to add.
   */
  __USED__ inline void addTask(Task *task)
  {
    // Increasing task count
    _taskCount++;

    // Checking if maximum was exceeded
    if (_taskCount >= MAX_SIMULTANEOUS_TASKS) HICR_THROW_LOGIC("Maximum task size (MAX_SIMULTANEOUS_TASKS = %lu) exceeded.\n", MAX_SIMULTANEOUS_TASKS);

    // Adding task to the waiting lis, it will be cleared out later
    _waitingTaskQueue.push(task);
  }

  /**
   * A callback function for HiCR to run upon the finalization of a given task. It adds the finished task's label to the finished task hashmap
   * (required for dependency management of any tasks that depend on this task) and terminates execution of the current worker if all tasks have
   * finished.
   *
   * \param[in] hicrTask Task to add.
   */
  __USED__ inline void onTaskFinish(HiCR::L1::tasking::Task *hicrTask)
  {
    // Getting TaskR task from HiCR task
    auto task = (Task *)hicrTask->getBackwardReferencePointer();

    // Decreasing overall task count
    _taskCount--;

    // Adding task label to finished task set
    _finishedTaskHashMap.insert(task->getLabel());

    // Free task's memory to prevent leaks. Could not use unique_ptr because the
    // type is not supported by boost's lock-free queue
    delete task;
  }

  /**
   * This function represents the main loop of a worker that is looking for work to do.
   * It first checks whether the maximum number of worker is exceeded. If that's the case, it enters suspension and returns upon restart.
   * Otherwise, it finds a task in the waiting queue and checks its dependencies. If the task is ready to go, it runs it.
   * If no tasks are ready to go, it returns a NULL, which encodes -No Task-.
   *
   * \return A pointer to a HiCR task to execute. NULL if there are no pending tasks.
   */
  __USED__ inline HiCR::L1::tasking::Task *checkWaitingTasks()
  {
    // Pointer to the next task to execute
    Task *task;

    // If all tasks finished, then terminate execution immediately
    if (_taskCount == 0)
    {
      // Getting a pointer to the currently executing worker
      auto worker = HiCR::L1::tasking::Worker::getCurrentWorker();

      // Terminating worker.
      worker->terminate();

      // Returning a NULL function
      return NULL;
    }

    // If maximum active workers is defined, then check if the threshold is exceeded
    checkMaximumActiveWorkerCount();

    // Poping next task from the lock-free queue
    bool foundTask = _waitingTaskQueue.try_pop(task);

    // If no task was found (queue was empty), then return an empty task
    if (foundTask == false)
      return NULL;

    // Check if task is ready now
    bool isTaskReady = checkTaskReady(task);

    // If it is, put it in the ready-to-go queue
    if (isTaskReady == true)
    {
      // If a task was found (queue was not empty), then execute and manage the
      // task depending on its state
      auto hicrTask = task->getHiCRTask();
      hicrTask->setEventMap(_eventMap);
      return hicrTask;
    }

    // Otherwise, put it at the back of the waiting task pile and return
    _waitingTaskQueue.push(task);

    // And return a null task
    return NULL;
  }

  /**
   * Starts the execution of the TaskR runtime.
   * Creates a set of HiCR workers, based on the provided computeManager, and subscribes them to a dispatcher queue.
   * After creating the workers, it starts them and suspends the current context until they're back (all tasks have finished).
   *
   * \param[in] computeManager The compute manager to use to coordinate the execution of processing units and tasks
   */
  __USED__ inline void run(HiCR::L1::ComputeManager *computeManager)
  {
    _dispatcher = new HiCR::L1::tasking::Dispatcher([this]()
                                                    { return checkWaitingTasks(); });

    _eventMap = new HiCR::L1::tasking::Task::taskEventMap_t();

    // Creating event map ands events
    _eventMap->setEvent(HiCR::L1::tasking::Task::event_t::onTaskFinish, [this](HiCR::L1::tasking::Task *task)
                        { onTaskFinish(task); });

    // Creating one worker per processung unit in the list
    for (auto &pu : _processingUnits)
    {
      // Creating new worker
      auto worker = new HiCR::L1::tasking::Worker(computeManager);

      // Assigning resource to the thread
      worker->addProcessingUnit(std::move(pu));

      // Assigning worker to the common dispatcher
      worker->subscribe(_dispatcher);

      // Finally adding worker to the worker set
      _workers.push_back(worker);

      // Initializing worker
      worker->initialize();
    }

    // Initializing active worker count
    _activeWorkerCount = _workers.size();

    // Starting workers
    for (auto &w : _workers) w->start();

    // Waiting for workers to finish
    for (auto &w : _workers) w->await();

    // Clearing created objects
    for (auto &w : _workers) delete w;
    _workers.clear();
    delete _dispatcher;
    delete _eventMap;
  }

  /**
   * Returns the currently executing TaskR task
   *
   * \return A pointer to the currently executing TaskR task
   */
  __USED__ inline Task *getCurrentTask() { return (Task *)HiCR::L1::tasking::Task::getCurrentTask()->getBackwardReferencePointer(); }

}; // class Runtime

} // namespace taskr
