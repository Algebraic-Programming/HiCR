#include <cstdio>
#include <chrono>
#include <hicr/core/L0/device.hpp>
#include <hicr/backends/host/L1/computeManager.hpp>
#include "../runtime.hpp"

static HiCR::backend::host::L1::ComputeManager *_computeManager;
static Runtime                                 *_runtime;
static std::atomic<uint64_t>                    _taskCounter;

// This function serves to encode a fibonacci task label
inline const uint64_t getFibonacciLabel(const uint64_t x, const uint64_t _initialValue) { return _initialValue; }

// Lookup table for the number of tasks required for every fibonacci number
uint64_t fibonacciTaskCount[] = {1,    1,    3,    5,     9,     15,    25,    41,    67,     109,    177,    287,    465,     753,     1219,   1973,
                                 3193, 5167, 8361, 13529, 21891, 35421, 57313, 92735, 150049, 242785, 392835, 635621, 1028457, 1664079, 2692537};

// Fibonacci without memoization to stress the tasking runtime
uint64_t fibonacci(const uint64_t x)
{
  if (x == 0) return 0;
  if (x == 1) return 1;

  uint64_t result1 = 0;
  uint64_t result2 = 0;
  auto     fibFc1  = _computeManager->createExecutionUnit([&]() { result1 = fibonacci(x - 1); });
  auto     fibFc2  = _computeManager->createExecutionUnit([&]() { result2 = fibonacci(x - 2); });

  uint64_t taskId1 = _taskCounter.fetch_add(1);
  uint64_t taskId2 = _taskCounter.fetch_add(1);

  auto task1 = new Task(taskId1, fibFc1);
  auto task2 = new Task(taskId2, fibFc2);

  _runtime->addTask(task1);
  _runtime->addTask(task2);

  _runtime->getCurrentTask()->addTaskDependency(taskId1);
  _runtime->getCurrentTask()->addTaskDependency(taskId2);

  _runtime->getCurrentTask()->suspend();

  return result1 + result2;
}

uint64_t fibonacciDriver(const uint64_t initialValue, HiCR::backend::host::L1::ComputeManager *computeManager, const HiCR::L0::Device::computeResourceList_t &computeResources)
{
  // Initializing runtime with the appropriate amount of max tasks
  Runtime runtime(fibonacciTaskCount[initialValue]);

  // Setting event handler to re-add task to the queue after it suspended itself
  runtime.setCallbackHandler(HiCR::tasking::Task::callback_t::onTaskSuspend, [&](HiCR::tasking::Task *task) { runtime.awakenTask(task); });

  // Setting global variables
  _runtime        = &runtime;
  _computeManager = computeManager;
  _taskCounter    = 0;

  // Assigning processing resource to TaskR
  for (const auto &computeResource : computeResources) runtime.addProcessingUnit(computeManager->createProcessingUnit(computeResource));

  // Storage for result
  uint64_t result = 0;

  // Creating task functions
  auto initialFc = computeManager->createExecutionUnit([&]() { result = fibonacci(initialValue); });

  // Now creating tasks and their dependency graph
  auto initialTask = new Task(_taskCounter.fetch_add(1), initialFc);
  runtime.addTask(initialTask);

  // Running runtime
  auto startTime = std::chrono::high_resolution_clock::now();
  runtime.run(computeManager);
  auto endTime     = std::chrono::high_resolution_clock::now();
  auto computeTime = std::chrono::duration_cast<std::chrono::duration<double>>(endTime - startTime);
  printf("Running Time: %0.5fs\n", computeTime.count());
  printf("Total Tasks: %lu\n", _taskCounter.load());

  // Returning fibonacci value
  return result;
}
