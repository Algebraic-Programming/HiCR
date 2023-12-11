/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file init.hpp
 * @brief Provides an initialization to the ACL runtime for the Ascend backend
 * @author L.Terracciano
 * @date 2/11/2023
 */

#pragma once

#include <acl/acl.h>
#include <hicr/backends/ascend/common.hpp>
#include <hicr/backends/sequential/L1/memoryManager.hpp>
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
 * and get the default context for each device.
 */
class Core final
{
  public:

  /**
   * Constructor for the core class for the ascend backend. It inizialies ACL
   *
   * \param configPath configuration file to initialize ACL
   */
  Core(const char *configPath = NULL) : _configPath(configPath){};

  /**
   * Default destructor
   */
  ~Core() = default;

  /**
   * Return the mapping between a device id and the ACL context for that device
   *
   * \return a map containing for each device Id its corresponding ascendState_t structure
   */
  __USED__ inline const std::unordered_map<deviceIdentifier_t, ascendState_t> &getContexts() const { return _deviceStatusMap; }

  /**
   * Discover available ascend devices, get memory information (HBM per single card), and create dedicated ACL contexts per device
   */
  void init()
  {
    aclError err = aclInit(_configPath);

    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Failed to initialize Ascend Computing Language. Error %d", err);
    // Discover and get default device contexts
    createContexts();

    // setup inter device communication
    setupInterDeviceCommunication();
  }

  /**
   * Finalize the ACL environment by destroying the device contexts
   */
  __USED__ inline void finalize()
  {
    aclError err = aclFinalize();
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Failed to initialize Ascend Computing Language. Error %d", err);
  }

  private:

  /**
   * Path to ACL config file
   */
  const char *_configPath;

  /**
   * Keeps track of how many devices are connected to the host
   */
  deviceIdentifier_t _deviceCount;

  /**
   * Keep track of the context for each deviceId
   */
  std::unordered_map<deviceIdentifier_t, ascendState_t> _deviceStatusMap;

  /**
   * Create ACL contexts for each available ascend device
   */
  __USED__ inline void createContexts()
  {
    // clear existing memory space map
    _deviceStatusMap.clear();

    // ask ACL for available devices
    aclError err;
    err = aclrtGetDeviceCount((uint32_t *)&_deviceCount);
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not retrieve ascend device count. Error %d", err);

    size_t ascendFreeMemory, ascendMemorySize;
    aclrtContext deviceContext;

    // add as many memory spaces as devices
    for (int32_t deviceId = 0; deviceId < (int32_t)_deviceCount; deviceId++)
    {
      // set the device
      err = aclrtSetDevice(deviceId);
      if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not select the ascend device %d. Error %d", deviceId, err);

      // retrieve the default device context
      err = aclrtGetCurrentContext(&deviceContext);
      if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not get default context in ascend device %d. Error %d", deviceId, err);

      // get the memory info
      err = aclrtGetMemInfo(ACL_HBM_MEM, &ascendFreeMemory, &ascendMemorySize);
      if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not retrieve ascend device %d memory space. Error %d", deviceId, err);

      // update the internal data structure
      _deviceStatusMap[deviceId] = ascendState_t{.context = deviceContext, .deviceType = deviceType_t::Npu, .size = ascendMemorySize};
    }
    // init host state (no context needed)
    const auto hostMemorySize = HiCR::backend::sequential::L1::MemoryManager::getTotalSystemMemory();
    _deviceStatusMap[(int32_t)_deviceCount] = ascendState_t{.deviceType = deviceType_t::Host, .size = hostMemorySize};
  }

  /**
   * Setup inter device communication in the ACL runtime.
   */
  __USED__ inline void setupInterDeviceCommunication()
  {
    int32_t canAccessPeer = 0;
    aclError err;

    // enable communication among each pair of ascend cards
    for (int32_t src = 0; src < (int32_t)_deviceCount; src++)
    {
      for (int32_t dst = 0; dst < (int32_t)_deviceCount; dst++)
      {
        if (src == dst) continue;

        // verify that the two cards can see each other
        err = aclrtDeviceCanAccessPeer(&canAccessPeer, src, dst);
        if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not determine peer accessibility to device %ld from device %ld.. Error %d", dst, src, err);

        if (canAccessPeer == 0) HICR_THROW_RUNTIME("Can not access device %ld from device %ld. Error %d", dst, src, err);

        selectDevice(_deviceStatusMap.at(dst).context, dst);

        // enable the communication
        err = aclrtDeviceEnablePeerAccess(src, 0);
        if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not enable peer access from device %ld to device %ld. Error %d", dst, src, err);
      }
    }
  }
};

} // namespace ascend

} // namespace backend

} // namespace HiCR