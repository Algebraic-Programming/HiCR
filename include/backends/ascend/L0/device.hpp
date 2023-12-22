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
    aclrtContext* context,
    const computeResourceList_t& computeResources,
    const memorySpaceList_t& memorySpaces
    ) : HiCR::L0::Device(computeResources, memorySpaces),  _id (id), _context(context)
    {};

  /**
   * Set the device on which the operations needs to be executed
   *
   * \param deviceContext the device ACL context
   * \param deviceId the device identifier
   */
  __USED__ static inline void selectDevice(const aclrtContext deviceContext, const deviceIdentifier_t deviceId)
  {
    // select the device context on which operations shoud be executed
    aclError err = aclrtSetCurrentContext(deviceContext);
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("can not set the device %ld context. Error %d", deviceId, err);
  }

  /**
   * Set this device as the one on which the operations needs to be executed
   */
  __USED__ inline void select() const
  {
    selectDevice(*_context, _id);
  }

  /**
   * Default destructor
   */
  ~Device() = default;

  __USED__ inline std::string getType() const override { return "Ascend Device"; }

  __USED__ inline deviceIdentifier_t getId() const { return _id; }
  __USED__ inline aclrtContext* getContext() const { return _context; }

  private:

  /**
   * Individual identifier for the ascend device
  */
  const deviceIdentifier_t _id;

  /**
   * The internal Ascend context associated to the device
   */
  aclrtContext* _context;

};

} // namespace L0

} // namespace ascend

} // namespace backend

} // namespace HiCR
