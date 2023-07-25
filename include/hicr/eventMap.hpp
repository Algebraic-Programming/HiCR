#pragma once

#include <map>

#include <hicr/common/logger.hpp>

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

class EventMap
{
 public:

  friend class Task;

  inline void clear()
  {
   _eventMap.clear();
  }

  inline void removeEvent(const event_t event)
  {
   _eventMap.erase(event);
  }

  inline void setEvent(const event_t event, eventCallback_t fc)
  {
   _eventMap[event] = fc;
  }

 private:

  // Only callable by tasks themselves
  inline void trigger(Task* task, const event_t event) const
  {
    if (_eventMap.contains(event)) _eventMap.at(event)(task);
  }


  std::map<event_t, eventCallback_t> _eventMap;
};

} // namespace HiCR

