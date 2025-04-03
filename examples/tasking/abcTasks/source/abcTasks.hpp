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
#include <hicr/core/L0/device.hpp>
#include <hicr/backends/pthreads/L1/computeManager.hpp>
#include "../runtime.hpp"

#define ITERATIONS 10

void abcTasks(Runtime &runtime)
{
  // Creating task functions
  auto taskAfc = [&runtime](void *arg) { printf("Task A %lu\n", ((Task *)arg)->getLabel()); };
  auto taskBfc = [&runtime](void *arg) { printf("Task B %lu\n", ((Task *)arg)->getLabel()); };
  auto taskCfc = [&runtime](void *arg) { printf("Task C %lu\n", ((Task *)arg)->getLabel()); };

  // Now creating tasks and their dependency graph
  for (size_t i = 0; i < ITERATIONS; i++)
  {
    auto cTask = new Task(i * 3 + 2, taskCfc);
    cTask->addTaskDependency(i * 3 + 1);
    runtime.addTask(cTask);
  }

  for (size_t i = 0; i < ITERATIONS; i++)
  {
    auto bTask = new Task(i * 3 + 1, taskBfc);
    bTask->addTaskDependency(i * 3 + 0);
    runtime.addTask(bTask);
  }

  for (size_t i = 0; i < ITERATIONS; i++)
  {
    auto aTask = new Task(i * 3 + 0, taskAfc);
    if (i > 0) aTask->addTaskDependency(i * 3 - 1);
    runtime.addTask(aTask);
  }

  // Running runtime
  runtime.run();
}
