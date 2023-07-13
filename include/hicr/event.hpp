#pragma once

#include <hicr/logger.hpp>
#include <hicr/common.hpp>

namespace HiCR
{

class Task;

// Definition for an event callback. It includes a reference to the finished task
typedef std::function<void(Task*)> eventCallback_t;

// Event types
enum event_t {
               onTaskExecute,
               onTaskYield,
               onTaskSuspend,
               onTaskFinish
             };

class Event
{
 public:

  Event(eventCallback_t fc) : _fc(fc) {}
  ~Event() = default;

  inline void trigger(Task* task)
  {
    _fc(task);
  }

 private:

  eventCallback_t _fc;
};

// Event map
typedef std::map<event_t, Event*> eventMap_t;

} // namespace HiCR

