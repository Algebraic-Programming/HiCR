/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file kernel.hpp
 * @brief This file implements the Kernel class for the ascend backend
 * @author S. M. Martin & L. Terracciano
 * @date 8/11/2023
 */

#pragma once

#include <acl/acl.h>
#include <hicr/common/definitions.hpp>
#include <hicr/common/exceptions.hpp>

namespace HiCR
{

namespace backend
{

namespace ascend
{

namespace kernel
{
  
/**
 * This class represents a replicable kernel for the ascend backend.
 * A Kernel is a piece of computation meant to be executed on an ascend device.
 */
class Kernel
{
  public:

  __USED__ inline Kernel(){};

  /**
   * Default destructor
   */
  __USED__ inline ~Kernel() = default;

  /**
   * Start the execution of the kernel on the given \p stream
   *
   * \param stream the stream on which the kernel is started
   */
  __USED__ inline virtual void start(const aclrtStream stream) = 0;
};

} // namespace kernel

} // namespace ascend

} // namespace backend

} // namespace HiCR
