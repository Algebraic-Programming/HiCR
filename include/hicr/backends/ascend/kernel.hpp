/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file kernel.hpp
 * @brief This file implements the Kernel class for the Ascend backend
 * @author S. M. Martin & L. Terracciano
 * @date 8/11/2023
 */

#pragma once

#include <acl/acl.h>
#include <hicr/core/definitions.hpp>

namespace HiCR
{

namespace backend
{

namespace ascend
{

/**
 * This class represents a replicable kernel for the Ascend backend.
 * A Kernel is a piece of computation meant to be executed on an Ascend device.
 */
class Kernel
{
  public:

  __INLINE__ Kernel() {};

  /**
   * Default destructor
   */
  __INLINE__ ~Kernel() = default;

  /**
   * Start the execution of the kernel on the given \p stream
   *
   * \param stream the stream on which the kernel is started
   */
  __INLINE__ virtual void start(const aclrtStream stream) = 0;
};

} // namespace ascend

} // namespace backend

} // namespace HiCR
