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

#include <memory>
#include <unordered_set>
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
  __USED__ inline const std::unordered_set<std::unique_ptr<HiCR::Instance>>& getInstances() const { return _instances; }

  protected:

  /**
  * Protected constructor; this is a purely abstract class
  */
  InstanceManager() = default;


  /**
   * Collection of instances
   */
  std::unordered_set<std::unique_ptr<HiCR::Instance>> _instances;
};

} // namespace backend

} // namespace HiCR
