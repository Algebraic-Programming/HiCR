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
#include <hccl/hccl.h>
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
 * and create context for each device
 */
class Initializer final
{
  public:

  /**
   * Constructor for the initializer class for the ascend backend. It inizialies ACL
   *
   * \param[in] config_path configuration file to initialize ACL
   */
  Initializer(const char *config_path = NULL)
  {
    aclError err = aclInit(NULL);

    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Failed to initialize Ascend Computing Language. Error %d", err);
  }

  ~Initializer()
  {
    // Destroy HCCL communicators among Ascends
    destroyHcclCommunicators();
    delete _hcclComms;
    for (const auto &deviceData : _deviceStatusMap) (void)aclrtDestroyContext(deviceData.second.context);
  }

  /**
   * return the mapping between a device Id and the ACL context for that device
   */
  __USED__ inline std::map<deviceIdentifier_t, ascendState_t> &getContexts() { return _deviceStatusMap; }

  /**
   * Discover available ascend devices, get memory information (HBM per single card), and create dedicated ACL contexts per device
   */
  void init()
  {
    // Discover and create device contexts
    createContexts();
    // setup HCCL communication
    setupHccl();
  }

  private:

  /**
   * Keeps track of how many devices are connected to the host
   */
  deviceIdentifier_t _deviceCount;

  /**
   * MPI-like communicators to transmit data among Ascends
   */
  HcclComm *_hcclComms = NULL;

  /**
   * Keep track of the context for each memorySpaceId/deviceId
   */
  std::map<deviceIdentifier_t, ascendState_t> _deviceStatusMap;

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
    for (uint32_t deviceId = 0; deviceId < _deviceCount; ++deviceId)
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
    _deviceStatusMap[_deviceCount] = ascendState_t{.deviceType = deviceType_t::Host, .size = hostMemorySize};
  }

  /**
   * Setup HCCL. This method populates the hccl communicators and initialize the peer communication
   * channels among ascends.
   */
  __USED__ inline void setupHccl()
  {
    if (_hcclComms == NULL) _hcclComms = new HcclComm[_deviceCount]();

    // destroy previously allocated hccl communicators
    destroyHcclCommunicators();

    HcclResult hcclErr;

    // instruct the HCCL api on how many devices ID are present
    int32_t devices[_deviceCount];
    for (uint32_t deviceId = 0; deviceId < _deviceCount; ++deviceId)
    {
      devices[deviceId] = deviceId;
    }

    // Setup a single-process multiple card communication
    hcclErr = HcclCommInitAll(_deviceCount, devices, _hcclComms);
    if (hcclErr != HCCL_SUCCESS) HICR_THROW_RUNTIME("Failed to initialize HCCL. Error %d", hcclErr);

    int32_t canAccessPeer = 0;
    aclError err;

    // enable communication among each pair of ascend cards
    for (uint32_t src = 0; src < _deviceCount; src++)
    {
      for (uint32_t dst = 0; dst < _deviceCount; dst++)
      {
        if (src == dst) continue;
        err = aclrtDeviceCanAccessPeer(&canAccessPeer, src, dst);
        if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not determine peer accessibility to device %ld from device %ld.. Error %d", dst, src, err);

        if (canAccessPeer == 0) HICR_THROW_RUNTIME("Can not access device %ld from device %ld. Error %d", dst, src, err);

        err = selectDevice(_deviceStatusMap.at(dst).context);
        if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not select the device %ld. Error %d", dst, err);

        err = aclrtDeviceEnablePeerAccess(src, 0);
        if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not enable peer access from device %ld to device %ld. Error %d", dst, src, err);
      }
    }
  }
  /**
   * Destroy the HCCL communicators used for the device-to-device communication.
   */
  __USED__ inline void destroyHcclCommunicators()
  {
    if (_hcclComms == NULL) return;

    for (uint32_t deviceId = 0; deviceId < _deviceCount; ++deviceId) (void)HcclCommDestroy(_hcclComms[deviceId]);
  }
};
} // namespace ascend
} // namespace backend
} // namespace HiCR