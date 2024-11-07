/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file callbackMap.hpp
 * @brief Provides a definition for the HiCR CallbackMap class.
 * @author S. M. Martin
 * @date 7/7/2023
 */

#pragma once

#include <map>
#include <hicr/core/definitions.hpp>
#include "./common.hpp"

namespace HiCR::tasking
{

/**
 * Defines a map that relates task-related callbacks to their corresponding callback.
 *
 * The callback is defined by the user and manually triggered by other (e.g., Task) classes, as the corresponding callback occurs.
 */
template <class T, class E>
class CallbackMap
{
  public:

  /**
   * Clears the callback map (no callbacks will be triggered)
   */
  __INLINE__ void clear() { _callbackMap.clear(); }

  /**
   * Remove a particular event's callback from the map
   *
   * \param[in] callback The callback to remove from the map
   */
  __INLINE__ void removeCallback(const E callback) { _callbackMap.erase(callback); }

  /**
   * Adds a callback for a particular event
   *
   * \param[in] callback The callback to add
   * \param[in] fc The callback function to call when the callback is triggered
   */
  __INLINE__ void setCallback(const E callback, callbackFc_t<T> fc) { _callbackMap[callback] = fc; }

  /**
   * Triggers the execution of the callback function for a given event
   *
   * \param[in] arg The argument to the trigger function.
   * \param[in] callback The triggered callback.
   */
  __INLINE__ void trigger(T arg, const E callback) const
  {
    if (_callbackMap.contains(callback)) _callbackMap.at(callback)(arg);
  }

  /**
   * Checks whether the specified callback is set
   *
   * \param[in] callback The callback to check for
   * \return True, if the callback has been set; false, otherwise
   */
  __INLINE__ bool isCallbackSet(const E callback) const { return _callbackMap.contains(callback); }

  private:

  /**
   * Internal storage for the callback map
   */
  std::map<E, callbackFc_t<T>> _callbackMap;
};

} // namespace HiCR::tasking
