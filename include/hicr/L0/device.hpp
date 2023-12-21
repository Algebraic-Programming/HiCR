/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file device.hpp
 * @brief Provides a base definition for a HiCR Device class
 * @author S. M. Martin
 * @date 18/12/2023
 */
#pragma once

#include <unordered_set>
#include <hicr/L0/computeResource.hpp>
#include <hicr/L0/memorySpace.hpp>

namespace HiCR
{

namespace L0
{

/**
 * This class represents an abstract definition for a HiCR Device that:
 *
 * - Represents a physical computing device (e.g., CPU+RAM, GPU+DRAM), containing a set of compute resources (e.g., cores) and/or memory spaces (e.g., RAM)
 * - It may contain information about the connectivity between its compute and memory resources
 * - This is a copy-able class that only contains metadata
 */
class Device
{
  public:

  /**
   * Common type for a collection of compute resources
   */
  typedef std::unordered_set<L0::ComputeResource*> computeResourceList_t;

  /**
   * Common definition of a collection of memory spaces
   */
  typedef std::unordered_set<L0::MemorySpace*> memorySpaceList_t;

  /**
   * Indicates what type of device is represented in this instance
   *
   * \return A string containing a human-readable description of the compute resource type
   */
  virtual std::string getType() const = 0;

  /**
   * This function returns the list of queried compute resources, as visible by the device.
   */
  __USED__ inline const computeResourceList_t& getComputeResourceList()  { return _computeResources; }

  /**
   * This function returns the list of queried memory spaces, as visible by the device.
   */
  __USED__ inline const memorySpaceList_t& getMemorySpaceList()  { return _memorySpaces; }

/**
   * This function allows the deferred addition of compute resources
   */
  __USED__ inline void addComputeResource(HiCR::L0::ComputeResource* const computeResource)  { _computeResources.insert(computeResource); }

  /**
   * This function allows the deferred addition of memory spaces
   */
  __USED__ inline void addMemorySpace(HiCR::L0::MemorySpace* const memorySpace)  { _memorySpaces.insert(memorySpace); }

  /**
   * Default destructor
   */
  virtual ~Device() = default;

 /**
  *  Constructor requires at least to provide the initial set of compute resources and memory spaces
  */
  Device(const computeResourceList_t& computeResources, const memorySpaceList_t& memorySpaces)
    : _computeResources(computeResources),
     _memorySpaces(memorySpaces)
    {} ;

  private:

  /**
  * Set of compute resources contained in this device.
  */
  computeResourceList_t _computeResources;

  /**
  * Set of memory spaces contained in this device.
  */
  memorySpaceList_t _memorySpaces;
};

} // namespace L0

} // namespace HiCR
