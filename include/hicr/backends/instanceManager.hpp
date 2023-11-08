/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file instanceManager.hpp
 * @brief Provides a definition for the abstract instance manager class
 * @author S. M. Martin
 * @date 2/11/2023
 */

#pragma once

#include <set>
#include <hicr/instance.hpp>

namespace HiCR
{

namespace backend
{

/**
 * Encapsulates a HiCR Backend Instance Manager.
 *
 * Backends need to fulfill the abstract virtual functions described here, so that HiCR can detect/create/communicate with other HiCR instances
 *
 */
class InstanceManager
{
  public:

  /**
   * Default destructor
   */
  virtual ~InstanceManager() = default;

  /**
   * This function prompts the backend to perform the necessary steps to discover and list the currently created (active or not)
   */
  __USED__ inline const std::set<HiCR::Instance*>& getInstances() const { return _instances; }

  /**
   * Function to retrieve the currently executing instance
   */
  __USED__ inline HiCR::Instance* getCurrentInstance() const { return _currentInstance; }

  /**
   * Function to check whether the current instance is the coordinator one (or just a worker)
   */
  virtual bool isCoordinatorInstance() = 0;


  /**
  * Protected constructor; this is a purely abstract class
  */
  InstanceManager() = default;

  /**
   * Collection of instances
   */
  std::set<HiCR::Instance*> _instances;

  /**
   * Pointer to current instance
   */
  HiCR::Instance* _currentInstance = NULL;

};

} // namespace backend

} // namespace HiCR
