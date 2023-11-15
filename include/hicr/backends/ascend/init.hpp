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
#include <hicr/backends/sequential/memoryManager.hpp>
#include <hicr/common/exceptions.hpp>
#include <map>

namespace HiCR
{

namespace backend
{

namespace ascend
{

/**
 * Initializer class implementation for the ascend backend responsible for initializing ACL
 * and create context for each device.
 */
class Initializer final
{
  public:

  /**
   * Constructor for the initializer class for the ascend backend. It inizialies ACL
   *
   * \param config_path configuration file to initialize ACL
   */
  Initializer(const char *config_path = NULL)
  {
    aclError err = aclInit(config_path);

    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Failed to initialize Ascend Computing Language. Error %d", err);
  }

  ~Initializer()
  {
    (void)aclFinalize();
  }

  /**
   * Return the mapping between a device id and the ACL context for that device
   *
   * \return a map containing for each device Id its corresponding ascendState_t structure
   */
  __USED__ inline const std::map<deviceIdentifier_t, ascendState_t> &getContexts() const { return _deviceStatusMap; }

  /**
   * Discover available ascend devices, get memory information (HBM per single card), and create dedicated ACL contexts per device
   */
  void init()
  {
    // Discover and create device contexts
    createContexts();

    // setup inter device communication
    setupInterDeviceCommunication();
  }

  /**
   * Finalize the ACL environment by destroying the device contexts
   */
  __USED__ inline void finalize()
  {
    for (const auto &deviceData : _deviceStatusMap) (void)aclrtDestroyContext(deviceData.second.context);
  }

  private:

  /**
   * Keeps track of how many devices are connected to the host
   */
  deviceIdentifier_t _deviceCount;

  /**
   * Keep track of the context for each deviceId
   */
  std::map<deviceIdentifier_t, ascendState_t> _deviceStatusMap;

  /**
   * Create ACL contexts for each available ascend device
   */
  __USED__ inline void createContexts()
  {
    // Clearing existing memory space map
    _deviceStatusMap.clear();

    // Ask ACL for available devices
    aclError err;
    err = aclrtGetDeviceCount((uint32_t *)&_deviceCount);
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not retrieve ascend device count. Error %d", err);

    size_t ascendFreeMemory, ascendMemorySize;
    aclrtContext deviceContext;

    // Add as many memory spaces as devices
    for (int32_t deviceId = 0; deviceId < (int32_t)_deviceCount; deviceId++)
    {
      // Create the device context
      err = aclrtCreateContext(&deviceContext, deviceId);
      if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not create context in ascend device %d. Error %d", deviceId, err);

      // Select the device by setting the context
      err = aclrtSetCurrentContext(deviceContext);
      if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not create context in ascend device %d. Error %d", deviceId, err);

      // Retrieve the memory info
      err = aclrtGetMemInfo(ACL_HBM_MEM, &ascendFreeMemory, &ascendMemorySize);
      if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not retrieve ascend device %d memory space. Error %d", deviceId, err);

      // update the internal data structure
      _deviceStatusMap[deviceId] = ascendState_t{.context = deviceContext, .deviceType = deviceType_t::Npu, .size = ascendMemorySize};
    }
    // init host state (no context needed)
    const auto hostMemorySize = HiCR::backend::sequential::MemoryManager::getTotalSystemMemory();
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