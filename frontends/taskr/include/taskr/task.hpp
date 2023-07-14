#pragma once

#include <taskr/common.hpp>
#include <taskr/runtime.hpp>
#include <hicr/task.hpp>
#include <vector>

namespace taskr
{

class Task
{

private:

 // HiCR Task object to implement TaskR tasks
 HiCR::Task _hicrTask;

 // Tasks's label, chosen by the user
 taskLabel_t _label;

 // Task dependency list
 std::vector<taskLabel_t> _taskDependencies;

public:

inline Task(const taskLabel_t label, const callback_t& fc) : _label(label)
 {
   _hicrTask.setFunction([fc](void* arg){ fc(); });
   _hicrTask.setArgument(this);
 }

 inline HiCR::Task* getHiCRTask()
 {
  return &_hicrTask;
 }

 inline taskLabel_t getLabel() const
 {
  return _label;
 }

 inline void addTaskDependency(const taskLabel_t task)
 {
  _taskDependencies.push_back(task);
 };

 inline bool isReady()
 {
  // Checking Task dependencies
  for (size_t i = 0; i < _taskDependencies.size(); i++)
   if (_runtime->_finishedTaskHashMap.contains(_taskDependencies[i]) == false) return false; // If any unsatisfied dependency was found, return immediately

  return true;
 }

}; // class Task

} // namespace taskr

