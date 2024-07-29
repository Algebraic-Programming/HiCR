/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file deployer.hpp
 * @brief This file implements the HiCR task class
 * @author Sergio Martin
 * @date 8/8/2023
 */

#pragma once

#include <hicr/core/concurrent/hashMap.hpp>
#include "common.hpp"

namespace HiCR
{

namespace tasking
{

/**
 * This class defines a generic implementation of an event dependency manager
 *
 * A dependency represents a dependent/depended relationship between an event and a callback. 
 *  * Dependents are represented by a counter that starts non-zero and is reduced by one every time one of its dependents is 'satisfied'
 *    * When the dependent's counter reaches zero, it executes an associated callback
 *  * Dependend events are set as 'satisfied' by manual calls to its respective function
 *  
 */
class DependencyManager final
{
  public:

  /**
   * Data type for identifying a unique event
   */
  typedef HiCR::tasking::uniqueId_t eventId_t;

  /**
   * Function type that serves as callback when a unique event is satisfied
   */
  typedef HiCR::tasking::callbackFc_t<eventId_t> eventCallbackFc_t;

  /**
   * Default constructor for the dependency manager
   * 
   * @param[in] eventTriggerCallback This is the callback that will be called whenever an event is triggered. That is, as soon as all it's dependencies have finished
   */
  DependencyManager(eventCallbackFc_t eventTriggerCallback)
    : _eventTriggerCallback(eventTriggerCallback)
  {}

  DependencyManager()  = delete;
  ~DependencyManager() = default;

  /**
 * Adds a new dependency between two unique events
 * 
 * @param[in] dependedId the event id that is depended upon
 * @param[in] dependentId the event id that has the dependency
 * 
 */
  __INLINE__ void addDependency(const eventId_t dependentId, const eventId_t dependedId)
  {
    // Increasing dependency counter of the dependent
    _inputDependencyCounterMap[dependentId]++;

    // Adding output dependency in the depended task
    _outputDependencyMap[dependedId].push_back(dependentId);
  }

  /**
   * Sets an event as satisfied
   * 
   * This function will also check for the event ids that depend on this event and trigger those who has all their dependencies now cleared.
   * 
   * @param[in] dependedId the event id to mark as satisfied
   */
  __INLINE__ void satisfyEvent(const eventId_t dependedId)
  {
    // For all dependents of the given depended id:
    for (const auto dependentId : _outputDependencyMap[dependedId])
    {
      // Decrease their input dependency counter
      _inputDependencyCounterMap[dependentId]--;

      // If the counter has reached zero:
      if (_inputDependencyCounterMap[dependentId] == 0)
      {
        // Remove it from the map (prevents uncontrolled memory use growth)
        _inputDependencyCounterMap.erase(dependentId);

        // And trigger the callback with the dependent id
        _eventTriggerCallback(dependentId);
      }

      // Removing output dependency from the map
      _outputDependencyMap.erase(dependedId);
    }
  }

  private:

  /**
   * This is a map that relates unique event ids to a counter of (input) dependencies that need to be satisfied before it is triggered
   */
  HiCR::concurrent::HashMap<eventId_t, size_t> _inputDependencyCounterMap;

  /**
   * This is a map that relates unique event ids to a set of its dependents (i.e., output dependencies).
   */
  HiCR::concurrent::HashMap<eventId_t, std::vector<eventId_t>> _outputDependencyMap;

  /**
   * The callback function to call when an event is triggered (i.e., has zero unsatisfied output dependencies)
   */
  const eventCallbackFc_t _eventTriggerCallback;

}; // class DependencyManager

} // namespace tasking

} // namespace HiCR
