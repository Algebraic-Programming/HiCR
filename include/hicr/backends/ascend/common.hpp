/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file common.hpp
 * @brief Provides a set of common type definitions for the Ascend backend
 * @author L.Terracciano
 * @date 2/11/2023
 */

#pragma once

#include <acl/acl.h>
#include <hicr/common/exceptions.hpp>

namespace HiCR
{

namespace backend
{

namespace ascend
{

/**
 * Type definition for the Ascend Device identifier
 */
typedef uint64_t deviceIdentifier_t;

/**
 * Enum used to determine if a device representation (ascendState_t) represents the host system or an Ascend card
 */
enum deviceType_t
{
  /**
   * Host system
   */
  Host = 0,
  /**
   * Ascend device
   */
  Npu
};

/**
 * Struct to keep trace of the information needed to perform operations on ascend and the memory size of both the devices and the host system
 */
struct ascendState_t
{
  /**
   * The context associated to a device
   */
  aclrtContext context;

  /**
   * Discriminate whether the context represents the host system or one of the Ascend devices
   */
  deviceType_t deviceType;

  /**
   * Memory size of the device
   */
  size_t size;

  aclrtStream stream;
};

/**
 * Set the device on which the operations needs to be executed
 *
 * \param deviceContext the device ACL context
 * \param deviceId the device identifier
 */
__USED__ inline void selectDevice(const aclrtContext deviceContext, const deviceIdentifier_t deviceId)
{
  // select the device context on which operations shoud be executed
  aclError err = aclrtSetCurrentContext(deviceContext);
  if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("can not set the device %ld context. Error %d", deviceId, err);
}
} // namespace ascend
} // namespace backend
} // namespace HiCR
