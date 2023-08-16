/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file eventMap.hpp
 * @brief Provides a definition for the HiCR EventMap class.
 * @author S. M. Martin
 * @date 7/7/2023
 */

#pragma once

#include <map>

#include <hicr/common/logger.hpp>

namespace HiCR
{

/**
 * Definition for an event callback. It includes a reference to the finished task
 */
typedef std::function<void(Task *)> eventCallback_t;

/**
 * Enumeration of possible task-related events
 */
enum event_t
{
  /**
   * Triggered as the task starts or resumes execution
   */
  onTaskExecute,

  /**
   * Triggered as the task voluntarily yields execution before finishing
   */
  onTaskYield,

  /**
   * Triggered as the task is preempted into suspension by an asynchronous event
   */
  onTaskSuspend,

  /**
   * Triggered as the task finishes execution
   */
  onTaskFinish
};

/**
 * Defines a map that relates task-related events to their corresponding callback.
 *
 * The callback is defined by the user and manually triggered by other (e.g., Task) classes, as the corresponding event occurs.
 */
class EventMap
{
  public:
  /**
   * Clears the event map (no events will be triggered)
   */
  inline void clear()
  {
    _eventMap.clear();
  }

  /**
   * Remove a particular event's callback from the map
   *
   * \param[in] event The event to remove from the map
   */
  inline void removeEvent(const event_t event)
  {
    _eventMap.erase(event);
  }

  /**
   * Adds a callback for a particular event
   *
   * \param[in] event The event to add
   * \param[in] fc The callback function to call when the event is triggered
   */
  inline void setEvent(const event_t event, eventCallback_t fc)
  {
    _eventMap[event] = fc;
  }

  /**
   * Triggers the execution of the callback function for a given event
   *
   * \param[in] task The triggering Task. This is necessary as parameter for the callback function type
   * \param[in] event The triggered event.
   */
  inline void trigger(Task *task, const event_t event) const
  {
    if (_eventMap.contains(event)) _eventMap.at(event)(task);
  }

  private:
  /**
   * Internal storage for the event map
   */
  std::map<event_t, eventCallback_t> _eventMap;
};

} // namespace HiCR
