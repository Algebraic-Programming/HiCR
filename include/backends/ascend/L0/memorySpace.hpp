/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file memorySpace.hpp
 * @brief This file implements the memory space class for the Ascend backend
 * @author S. M. Martin & L. Terracciano
 * @date 15/12/2023
 */

#pragma once

#include <backends/ascend/common.hpp>
#include <backends/ascend/L0/memorySlot.hpp>
#include <hicr/L0/memorySpace.hpp>
#include <hicr/common/definitions.hpp>

namespace HiCR
{

namespace backend
{

namespace ascend
{

namespace L0
{

/** 
 * This class represents a memory space, as visible by the sequential backend. That is, the entire RAM that the running CPU has access to.
 */
class MemorySpace final : public HiCR::L0::MemorySpace
{
  public:

  /**
   * Constructor for the compute resource class of the sequential backend
   */
  MemorySpace(const size_t size,
   const ascend::deviceType_t deviceType,
   const ascend::deviceIdentifier_t deviceId) :
   HiCR::L0::MemorySpace(size),
   _deviceType(deviceType),
   _deviceId(deviceId)
     {};

  /**
   * Default destructor
   */
  ~MemorySpace() = default;

  __USED__ inline std::string getType() const override { return "NUMA Domain"; }


  /**
   * Function to get the device type associated to this memory space
   *
   * @return The device type corresponding to this memory space
   */
  __USED__ inline const ascend::deviceType_t getDeviceType() const
  {
    return _deviceType;
  }

  /**
  * Function to get the device id associated to this memory space
  *
  * @return The device id corresponding to this memory space
  */
  __USED__ inline const ascend::deviceIdentifier_t getDeviceId() const
  {
    return _deviceId;
  }

  private:

  /**
   * Stores the device type that this memory space belongs to
   */
  const ascend::deviceType_t _deviceType;

  /**
   * Stores the device identifier that hold this memory space. This might need to be removed in favor of defining
   * a device class
  */
 const ascend::deviceIdentifier_t _deviceId;
};

} // namespace L0

} // namespace ascend

} // namespace backend

} // namespace HiCR
