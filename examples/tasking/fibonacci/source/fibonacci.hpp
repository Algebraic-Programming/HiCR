/*
 *   Copyright 2025 Huawei Technologies Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cstdio>
#include <chrono>
#include <hicr/core/device.hpp>
#include <hicr/backends/pthreads/computeManager.hpp>
#include "../runtime.hpp"

static Runtime              *_runtime;
static std::atomic<uint64_t> _taskCounter;

// This function serves to encode a fibonacci task label
inline const uint64_t getFibonacciLabel(const uint64_t x, const uint64_t _initialValue) { return _initialValue; }

// Lookup table for the number of tasks required for every fibonacci number
uint64_t fibonacciTaskCount[] = {1,    1,    3,    5,     9,     15,    25,    41,    67,     109,    177,    287,    465,     753,     1219,   1973,
                                 3193, 5167, 8361, 13529, 21891, 35421, 57313, 92735, 150049, 242785, 392835, 635621, 1028457, 1664079, 2692537};

// Fibonacci without memoization to stress the tasking runtime
uint64_t fibonacci(Task *currentTask, const uint64_t x)
{
  if (x == 0) return 0;
  if (x == 1) return 1;

  uint64_t result1 = 0;
  uint64_t result2 = 0;
  auto     fibFc1  = [&](void *arg) { result1 = fibonacci((Task *)arg, x - 1); };
  auto     fibFc2  = [&](void *arg) { result2 = fibonacci((Task *)arg, x - 2); };

  uint64_t taskId1 = _taskCounter.fetch_add(1);
  uint64_t taskId2 = _taskCounter.fetch_add(1);

  auto task1 = new Task(taskId1, fibFc1);
  auto task2 = new Task(taskId2, fibFc2);

  _runtime->addTask(task1);
  _runtime->addTask(task2);

  currentTask->addTaskDependency(taskId1);
  currentTask->addTaskDependency(taskId2);

  currentTask->suspend();

  return result1 + result2;
}

uint64_t fibonacciDriver(Runtime &runtime, const uint64_t initialValue)
{
  // Setting event handler to re-add task to the queue after it suspended itself
  runtime.setCallbackHandler(HiCR::tasking::Task::callback_t::onTaskSuspend, [&](HiCR::tasking::Task *task) { runtime.awakenTask(task); });

  // Setting global variables
  _runtime     = &runtime;
  _taskCounter = 0;

  // Storage for result
  uint64_t result = 0;

  // Creating task functions
  auto initialFc = [&](void *arg) { result = fibonacci((Task *)arg, initialValue); };

  // Now creating tasks and their dependency graph
  auto initialTask = new Task(_taskCounter.fetch_add(1), initialFc);
  runtime.addTask(initialTask);

  // Running runtime
  auto startTime = std::chrono::high_resolution_clock::now();
  runtime.run();
  auto endTime     = std::chrono::high_resolution_clock::now();
  auto computeTime = std::chrono::duration_cast<std::chrono::duration<double>>(endTime - startTime);
  printf("Running Time: %0.5fs\n", computeTime.count());
  printf("Total Tasks: %lu\n", _taskCounter.load());

  // Returning fibonacci value
  return result;
}
