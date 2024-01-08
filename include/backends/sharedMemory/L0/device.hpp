/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file device.hpp
 * @brief This file implements the abstract device class for the host (CPU) backends
 * @author S. M. Martin
 * @date 18/12/2023
 */

#pragma once

#include <hicr/definitions.hpp>
#include <hicr/L0/device.hpp>
#include <hicr/L0/computeResource.hpp>
#include <hicr/L0/memorySpace.hpp>

namespace HiCR
{

namespace backend
{

namespace sharedMemory
{

namespace L0
{

/**
 * This class represents a host (CPU) NUMA Domain containing within itself a subset of the whole host RAM and processing units with uniform access time to it
 */
class Device : public HiCR::L0::Device
{
  public:

  /**
   * Type definition for a NUMA Domain identifier
  */
  typedef int NUMADomainID_t; 

  /**
   * Constructor for the device class of the sequential backend
   *
   * @param[in] NUMADomainId The OS-given NUMA domain identifier represented by this class
   * @param[in] computeResources The compute resources (cores or hyperthreads) detected in this device (CPU)
   * @param[in] memorySpaces The memory spaces (e.g., NUMA domains) detected in this device (CPU)
   */
  Device(const NUMADomainID_t NUMADomainId, const computeResourceList_t &computeResources, const memorySpaceList_t &memorySpaces) : 
  HiCR::L0::Device(computeResources, memorySpaces),
  _NUMADomainId(NUMADomainId) {};

  protected:

  /**
   * Protected empty constructor for deserialization
  */
  Device() :  HiCR::L0::Device() {};

  /**
   * Default destructor
   */
  ~Device() = default;

  __USED__ inline std::string getType() const override { return "NUMA Domain"; }

  /**
   * Identifier for the NUMA domain represented by this class
  */
  NUMADomainID_t _NUMADomainId;
};

} // namespace L0

} // namespace sharedMemory

} // namespace backend

} // namespace HiCR
