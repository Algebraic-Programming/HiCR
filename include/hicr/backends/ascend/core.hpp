/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file core.hpp
 * @brief Provides an initialization to the ACL runtime for the Ascend backend
 * @author L.Terracciano
 * @date 2/11/2023
 */

#pragma once

#include <acl/acl.h>
#include <backends/ascend/common.hpp>
#include <backends/sequential/L1/memoryManager.hpp>
#include <hicr/common/exceptions.hpp>
#include <unordered_map>

namespace HiCR
{

namespace backend
{

namespace ascend
{

/**
 * Core class implementation for the ascend backend responsible for initializing ACL
 * 
 */
class Core final
{
  public:

  /**
   * Constructor for the core class for the ascend backend
   *
   * @param[in] configPath Path to the configuration file to use for ACL. NULL sets a default configuration.
   *
   */
  Core(const char *configPath = NULL)
    : _configPath(configPath){};

  /**
   * Default destructor
   */
  ~Core() = default;

  /**
   * Initializes ACL runtime
   *
   * @param[in] configPath configuration file to initialize ACL
   */
  void init()
  {
    aclError err = aclInit(_configPath);
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Failed to initialize Ascend Computing Language. Error %d", err);
  }

  /**
   * Finalize the ACL runtime
   */
  __INLINE__ void finalize()
  {
    aclError err = aclFinalize();
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Failed to finalize Ascend Computing Language. Error %d", err);
  }

  private:

  /**
   * Path to ACL config file
   */
  const char *_configPath;
};

} // namespace ascend

} // namespace backend

} // namespace HiCR