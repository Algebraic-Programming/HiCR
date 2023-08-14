/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

#pragma once

#include <vector>
#include <hicr/task.hpp>
#include <hicr/common/taskQueue.hpp>

namespace HiCR
{

typedef std::function<Task*(void)> pullFunction_t;

class Dispatcher
{
 private:

 // Pull function, if defined by the user
 pullFunction_t _pullFc;
 bool isPullFunctionDefined = false;

 // Internal queue for pushed tasks
 common::taskQueue _queue;

 public:

 Dispatcher() = default;
 ~Dispatcher() = default;

 inline void setPullFunction(const pullFunction_t pullFc)
 {
  _pullFc = pullFc;
  isPullFunctionDefined = true;
 }

 inline void clearPullFunction()
 {
  isPullFunctionDefined = false;
 }

 inline Task* pull()
 {
  if (isPullFunctionDefined == false) LOG_ERROR("Trying to pull on dispatcher but the pull function is not defined\n");
  return _pullFc();
 }

 inline void push(Task* task)
 {
  _queue.push(task);
 }

 inline Task* pop()
 {
  return _queue.pop();
 }

 inline Task* pullOrPop()
 {
  Task* task = pop();

  if (isPullFunctionDefined == true && task == NULL) task = pull();

  return task;
 }
};

} // namespace HiCR

