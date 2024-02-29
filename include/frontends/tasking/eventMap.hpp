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
#include <hicr/definitions.hpp>

namespace HiCR
{

namespace tasking
{

/**
 * Definition for an event callback. It includes a reference to the finished task
 */
template <class T>
using eventCallback_t = std::function<void(T *)>;

/**
 * Defines a map that relates task-related events to their corresponding callback.
 *
 * The callback is defined by the user and manually triggered by other (e.g., Task) classes, as the corresponding event occurs.
 */
template <class T, class E>
class EventMap
{
  public:

  /**
   * Clears the event map (no events will be triggered)
   */
  __USED__ inline void clear()
  {
    _eventMap.clear();
  }

  /**
   * Remove a particular event's callback from the map
   *
   * \param[in] event The event to remove from the map
   */
  __USED__ inline void removeEvent(const E event)
  {
    _eventMap.erase(event);
  }

  /**
   * Adds a callback for a particular event
   *
   * \param[in] event The event to add
   * \param[in] fc The callback function to call when the event is triggered
   */
  __USED__ inline void setEvent(const E event, eventCallback_t<T> fc)
  {
    _eventMap[event] = fc;
  }

  /**
   * Triggers the execution of the callback function for a given event
   *
   * \param[in] arg The argument to the trigger function.
   * \param[in] event The triggered event.
   */
  __USED__ inline void trigger(T *arg, const E event) const
  {
    if (_eventMap.contains(event)) _eventMap.at(event)(arg);
  }

  private:

  /**
   * Internal storage for the event map
   */
  std::map<E, eventCallback_t<T>> _eventMap;
};

} // namespace tasking

} // namespace HiCR
