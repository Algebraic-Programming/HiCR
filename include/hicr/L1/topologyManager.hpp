/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file deviceManager.hpp
 * @brief Provides a definition for the abstract device manager class
 * @author S. M. Martin
 * @date 18/12/2023
 */

#pragma once

#include <memory>
#include <hicr/L0/device.hpp>
#include <hicr/L1/memoryManager.hpp>

namespace HiCR
{

namespace L1
{

/**
 * Encapsulates a HiCR Backend Device Manager.
 *
 * The purpose of this class is to discover the computing topology for a given device type.
 * E.g., if this is a backend for an NPU device and the system contains 8 such devices, it will discover an
 * array of Device type of size 8.
 *
 * Backends need to fulfill the abstract virtual functions described here, so that HiCR can detect hardware devices
 */
class TopologyManager
{
  public:

  /**
   * Common type for a collection of devices
   */
  typedef std::unordered_set<std::shared_ptr<L0::Device>> deviceList_t;

  /**
   * Default constructor is allowed, as no default argument are expected for the creation of this class
   */
  TopologyManager() = default;

  /**
   * Default destructor
   */
  virtual ~TopologyManager() = default;

  /**
   * This function prompts the backend to perform the necessary steps to discover and list the compute units provided by the library which it supports.
   *
   * In case of change in resource availability during runtime, users need to re-run this function to be able to see the changes.
   *
   * \internal It does not return anything because we want to allow users to run only once, and then consult it many times without having to make a copy.
   */
  __USED__ inline void queryDevices()
  {
    // Clearing existing compute units
    _deviceList.clear();

    // Calling backend-internal implementation
    _deviceList = queryDevicesImpl();
  }

  /**
   * This function prompts the backend to perform the necessary steps to return  existing devices
   * \return A set of pointers to HiCR instances that refer to both local and remote instances
   */
  __USED__ inline const deviceList_t &getDevices() const { return _deviceList; }

  protected:

  /**
   * Backend-specific implementation of queryDevices
   *
   * \return The list of devices detected by this backend
   */
  virtual deviceList_t queryDevicesImpl() = 0;

  private:

  /**
   * Map of execution units, representing potential RPC requests
   */
  deviceList_t _deviceList;
};

} // namespace L1

} // namespace HiCR
