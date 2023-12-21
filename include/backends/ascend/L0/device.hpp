/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file device.hpp
 * @brief This file implements the Device class for the Ascend backend
 * @author L. Terracciano & S. M. Martin
 * @date 20/12/2023
 */

#pragma once

#include <acl/acl.h>
#include <hicr/L0/device.hpp>
#include <backends/ascend/L0/computeResource.hpp>
#include <backends/ascend/L0/memorySpace.hpp> 
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
 * This class represents a device, as visible by the shared memory backend. That is, an assumed SMP processor plus a shared RAM that all process have access to.
 */
class Device final : public HiCR::L0::Device
{
  public:

  /**
   * Type definition for the Ascend Device identifier
   */
  typedef uint64_t deviceIdentifier_t;

  /**
   * Constructor for the device class of the sequential backend
   */
  Device(
    const deviceIdentifier_t id,
    const aclrtContext context,
    const computeResourceList_t& computeResources,
    const memorySpaceList_t& memorySpaces
    ) : HiCR::L0::Device(computeResources, memorySpaces),  _id (id), _context(context)
    {};

  /**
   * Default destructor
   */
  ~Device() = default;

  __USED__ inline std::string getType() const override { return "Ascend Device"; }

  __USED__ inline deviceIdentifier_t getId() const { return _id; }
  __USED__ inline aclrtContext getContext() const { return _context; }

  private:

  /**
   * Individual identifier for the ascend device
  */
  const deviceIdentifier_t _id;

  /**
   * The internal Ascend context associated to the device
   */
  const aclrtContext _context;

};

} // namespace L0

} // namespace ascend

} // namespace backend

} // namespace HiCR
