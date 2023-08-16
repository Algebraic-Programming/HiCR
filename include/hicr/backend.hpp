/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file backend.hpp
 * @brief Provides a definition for the backend class.
 * @author S. M. Martin
 * @date 27/6/2023
 */

#pragma once

#include <hicr/resource.hpp>

namespace HiCR
{

class Runtime;

/**
 * Encapsulates a HiCR Backend.
 *
 * Backends represent plugins to HiCR that provide support for a communication or device library. By adding new plugins developers extend HiCR's support for new hardware and software technologies.
 *
 * Backends need to fulfill the abstract virtual functions described here, so that HiCR can perform common operations on the supported device/network library.
 *
 */
class Backend
{
  friend class Runtime;

  protected:

  Backend() = default;

  public:

  virtual ~Backend() = default;

  /**
   * This function prompts the backend to perform the necessary steps to discover and list the resources provided by the library which it supports.
   *
   * In case of change in resource availability during runtime, users need to re-run this function to be able to see the changes.
   *
   * \internal It does not return anything because we want to allow users to run only once, and then consult it many times without having to make a copy.
   */
  virtual void queryResources() = 0;

  /**
   * This function returns the list of queried resources, as obtained by.
   *
   * If this function is called before queryResources, then it will return an empty container.
   *
   * @return The list of resources, as detected the last time \a queryResources was executed.
   */
  inline resourceList_t &getResourceList() { return _resourceList; }

  protected:

  /**
   * The internal container for the queried resources.
   */
  resourceList_t _resourceList;
};

} // namespace HiCR
