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

#include <hicr/common/definitions.hpp>
#include <hicr/computeResource.hpp>
#include <hicr/memorySpace.hpp>

namespace HiCR
{

class Runtime;

/**
 * Common definition of a collection of compute resources
 */
typedef std::vector<std::unique_ptr<ComputeResource>> computeResourceList_t;

/**
 * Common definition of a collection of memory spaces
 */
typedef std::vector<std::unique_ptr<MemorySpace>> memorySpaceList_t;

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
  __USED__ virtual void queryResources() = 0;

  /**
   * This function returns the list of queried compute resources, as visible by the backend.
   *
   * If this function is called before queryResources, then it will return an empty container.
   *
   * @return The list of compute resources, as detected the last time \a queryResources was executed.
   */
  __USED__ inline computeResourceList_t &getComputeResourceList() { return _computeResourceList; }

  /**
   * This function returns the list of queried memory spaces, as visible by the backend.
   *
   * If this function is called before queryResources, then it will return an empty container.
   *
   * @return The list of memory spaces, as detected the last time \a queryResources was executed.
   */
  __USED__ inline memorySpaceList_t &getMemorySpaceList() { return _memorySpaceList; }

  protected:

  /**
   * The internal container for the queried resources.
   */
  computeResourceList_t _computeResourceList;

  /**
   * The internal container for the queried resources.
   */
  memorySpaceList_t _memorySpaceList;
};

} // namespace HiCR
