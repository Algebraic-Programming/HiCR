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
 * Enum used to determine if an ascendState_t represents the host system or an Ascend card
 */
enum deviceType_t
{
  Host = 0,
  Npu
};

/**
 * Struct to keep trace of the information needed to perform operation on ascend device and host memory
 */
struct ascendState_t
{
  /**
   * remember the context associated to a device
   */
  aclrtContext context;

  /**
   * Tells whether the context represents the host system or the Ascend
   */
  deviceType_t deviceType;

  /**
   * memory size of the device
   */
  size_t size;
};

/**
 * Set the device on which the operations needs to be executed
 *
 * \param[in] deviceContext the device acl context
 */
__USED__ inline aclError selectDevice(const aclrtContext deviceContext)
{
  // select the device context on which operations shoud be executed
  return aclrtSetCurrentContext(deviceContext);
}
} // namespace ascend
} // namespace backend
} // namespace HiCR
