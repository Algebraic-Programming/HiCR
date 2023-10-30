/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file memoryManager.hpp
 * @brief This file implements the memory manager class for the Ascend backend
 * @author S. M. Martin and L. Terracciano
 * @date 11/9/2023
 */

#pragma once

#include <acl/acl.h>
#include <hccl/hccl.h>
#include <hicr/backends/ascend/memorySlot.hpp>
#include <hicr/backends/memoryManager.hpp>
#include <hicr/backends/sequential/memoryManager.hpp>
#include <hicr/common/definitions.hpp>

namespace HiCR
{

namespace backend
{

namespace ascend
{

/**
 * Implementation of the HiCR MPI backend
 *
 * This backend is very useful for testing other HiCR modules in isolation (unit tests) without involving the use of threading, which might incur side-effects
 */
class MemoryManager final : public backend::MemoryManager
{
  public:

  /**
   * Constructor for the ascend backend.
   *
   * \param[in] config_path configuration file to initialize ACL
   */
  MemoryManager(const char *config_path = NULL) : HiCR::backend::MemoryManager()
  {
    aclError err;
    err = aclInit(config_path);

    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Failed to initialize Ascend Computing Language. Error %d", err);
  }

  ~MemoryManager()
  {
    // Destroy HCCL communicators among Ascends
    destroyHcclCommunicators();
    free(hcclComms);

    // Destroy Ascend contexts
    for (const auto memSpaceData : _deviceStatusMap)
    {
      (void)aclrtDestroyContext(memSpaceData.second.context);
    }

    _deviceStatusMap.clear();

    // Finalize ACL environment
    (void)aclFinalize();
  }

  private:

  /**
   * Enum used to determine if an ascendState_t represents the host system or an Ascend card
   */
  enum deviceType_t
  {
    Host = 0,
    Npu
  };

  /**
   * Keeps track of how many devices are connected to the host
   */
  uint32_t deviceCount;

  /**
   * MPI-like communicators to transmit data among Ascends
   */
  HcclComm *hcclComms = NULL;

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
   * Keep track of the context for each memorySpaceId/deviceId
   */
  std::map<memorySpaceId_t, ascendState_t> _deviceStatusMap;

  /**
   * Set the device on which the operations needs to be executed
   *
   * \param[in] memorySpace the device identifier
   */
  __USED__ inline void selectDevice(const memorySpaceId_t memorySpace)
  {
    aclError err;

    // select the device context on which we should allocate the memory
    err = aclrtSetCurrentContext(_deviceStatusMap[memorySpace].context);
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not set ascend device %d. Error %d", memorySpace, err);
  }

  /**
   * This function returns the available allocatable size in the current system RAM
   *
   * @param[in] memorySpace Either the memory space representing the device or the host
   * @return The allocatable size within the system
   */
  __USED__ inline size_t getMemorySpaceSizeImpl(const memorySpaceId_t memorySpace) const override
  {
    const auto deviceState = _deviceStatusMap.at(memorySpace);

    return deviceState.size;
  }

  /**
   * Ascend backend implementation that returns a memory space representing the entire RAM device memory and the
   * other ones representing the Ascends connected to the host. Once the memory spaces are discovered, the HCCL
   * communicators are configured to enable device-to-device communication.
   *
   * \return a list of memory spaces representing the system status (host memory + ascend devices connected)
   */
  __USED__ inline memorySpaceList_t queryMemorySpacesImpl() override
  {
    // Discover memory spaces
    const auto memorySpaceList = createMemorySpacesListAndSetupContexts();

    // setup HCCL communication
    setupHccl();

    return memorySpaceList;
  }

  /**
   * Destroy the HCCL communicators used for the device-to-device communication.
   */
  __USED__ inline void destroyHcclCommunicators()
  {
    if (hcclComms == NULL) return;

    for (uint32_t deviceId = 0; deviceId < deviceCount; ++deviceId)
    {
      (void)HcclCommDestroy(hcclComms[deviceId]);
    }
  }

  /**
   * Setup HCCL. This method populates the hccl communicators and initialize the peer communication
   * channels among ascends.
   */
  __USED__ inline void setupHccl()
  {
    if (hcclComms == NULL) hcclComms = new HcclComm[deviceCount]();

    // destroy previously allocated hccl communicators
    destroyHcclCommunicators();

    HcclResult hcclErr;

    // instruct the HCCL api on how many devices ID are present
    int32_t devices[deviceCount];
    for (uint32_t deviceId = 0; deviceId < deviceCount; ++deviceId)
    {
      devices[deviceId] = deviceId;
    }

    // Setup a single-process multiple card communication
    hcclErr = HcclCommInitAll(deviceCount, devices, hcclComms);
    if (hcclErr != HCCL_SUCCESS) HICR_THROW_RUNTIME("Failed to initialize HCCL. Error %d", hcclErr);

    int32_t canAccessPeer = 0;
    aclError err;

    // enable communication among each pair of ascend cards
    for (uint32_t src = 0; src < deviceCount; src++)
    {
      for (uint32_t dst = 0; dst < deviceCount; dst++)
      {
        if (src == dst) continue;
        err = aclrtDeviceCanAccessPeer(&canAccessPeer, src, dst);
        if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not determine peer accessibility to device %ld from device %ld.. Error %d", dst, src, err);

        if (canAccessPeer == 0) HICR_THROW_RUNTIME("Can not access device %ld from device %ld. Error %d", dst, src, err);

        selectDevice(dst);
        err = aclrtDeviceEnablePeerAccess(src, 0);
        if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not enable peer access from device %ld to device %ld. Error %d", dst, src, err);
      }
    }
  }

  /**
   * Ascend backend implementation that returns a list of memory space representing the host memory and the Ascend cards with the context already initialized.
   *
   * \return A list of memory spaces that comprise both the Ascend devices and the host.
   */
  __USED__ inline memorySpaceList_t createMemorySpacesListAndSetupContexts()
  {
    // Clearing existing memory space map
    _deviceStatusMap.clear();

    // New memory space list to return
    memorySpaceList_t memorySpaceList;

    // Ask ACL for available devices
    aclError err;
    err = aclrtGetDeviceCount(&deviceCount);
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not retrieve ascend device count. Error %d", err);

    size_t ascendFreeMemory, ascendMemorySize;
    aclrtContext deviceContext;

    // Add as many memory spaces as devices
    for (uint32_t deviceId = 0; deviceId < deviceCount; ++deviceId)
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
      memorySpaceList.insert(deviceId);
    }

    // init host context
    const auto hostMemorySize = HiCR::backend::sequential::MemoryManager::getTotalSystemMemory();
    _deviceStatusMap[deviceCount] = ascendState_t{.deviceType = deviceType_t::Host, .size = hostMemorySize};
    memorySpaceList.insert(deviceCount);

    return memorySpaceList;
  }

  /**
   * Backend-internal implementation of the queryLocalMemorySlot function
   *
   * \param[in] memorySpaceId Memory space to allocate memory in
   * \param[in] size Size of the memory slot to create
   * \return The internal pointer associated to the local memory slot
   */
  __USED__ inline MemorySlot *allocateLocalMemorySlotImpl(const memorySpaceId_t memorySpaceId, const size_t size) override
  {
    void *ptr = NULL;
    if (_deviceStatusMap.at(memorySpaceId).deviceType == deviceType_t::Host)
    {
      ptr = hostAlloc(size);
    }
    else
    {
      ptr = deviceAlloc(memorySpaceId, size);
    }

    // keep track of the mapping between unique identifier and pointer + deviceId
    auto memorySlot = new MemorySlot(memorySpaceId, ptr, size);

    return memorySlot;
  }
  /**
   * Allocate memory on the Host memory through Ascend-dedicated functions.
   *
   * \param[in] size Allocation size
   */
  __USED__ inline void *hostAlloc(const size_t size)
  {
    void *ptr;

    // do the allocation on host memory
    aclError err = aclrtMallocHost(&ptr, size);
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not allocate memory on ascend host. Error %d", err);

    return ptr;
  }

  /**
   * Allocate memory on the Ascend memory through Ascend-dedicated functions.
   *
   * \param[in] memorySpace Device id where memory is allocated
   * \param[in] size Allocation size
   */
  __USED__ inline void *deviceAlloc(const memorySpaceId_t memorySpace, const size_t size)
  {
    void *ptr;

    // select the device context on which we should allocate the memory
    selectDevice(memorySpace);

    // do the allocation on device memory
    aclError err = aclrtMalloc(&ptr, size, ACL_MEM_MALLOC_HUGE_FIRST);
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not allocate memory on ascend device %d. Error %d", memorySpace, err);

    return ptr;
  }
  /**
   * Backend-internal implementation of the registerLocalMemorySlot function
   *
   * \param[in] ptr Pointer to the start of the memory slot
   * \param[in] size Size of the memory slot to create
   * \return A newly created memory slot
   */
  __USED__ inline MemorySlot *registerLocalMemorySlotImpl(void *const ptr, const size_t size) override
  {
    HICR_THROW_RUNTIME("Not yet implemented for this backend");
  }

  /**
   * Backend-internal implementation of the freeLocalMemorySlot function
   *
   * \param[in] memorySlot Local memory slot to free up. It becomes unusable after freeing.
   */
  __USED__ inline void freeLocalMemorySlotImpl(HiCR::MemorySlot *memorySlot) override
  {
    // Getting up-casted pointer for the execution unit
    auto m = dynamic_cast<MemorySlot *>(memorySlot);

    // Checking whether the execution unit passed is compatible with this backend
    if (m == NULL) HICR_THROW_LOGIC("The passed memory slot is not supported by this backend\n");

    // Getting memory slot info
    const auto memorySlotPointer = m->getPointer();
    const auto memorySlotDeviceId = m->getDeviceId();

    if (_deviceStatusMap.at(memorySlotDeviceId).deviceType == deviceType_t::Host)
    {
      freeHostMemorySlot(memorySlotPointer);
    }
    else
    {
      freeDeviceMemorySlot(memorySlotDeviceId, memorySlotPointer);
    }
  }

  /**
   * Release memory on the Host memory through Ascend-dedicated functions.
   *
   * \param[in] ptr Local memory to free up.
   */
  __USED__ inline void freeHostMemorySlot(const void *ptr)
  {
    aclError err;
    err = aclrtFreeHost((void *)ptr);
    if (err != ACL_SUCCESS) HICR_THROW_LOGIC("Error while freeing host memory. Error %d", err);
  }

  /**
   * Release memory on the Ascend memory through Ascend-dedicated functions.
   *
   * \param[in] deviceId Device id where memory is allocated.
   * \param[in] ptr Local memory to free up.
   */
  __USED__ inline void freeDeviceMemorySlot(const deviceIdentifier_t deviceId, const void *ptr)
  {
    selectDevice(deviceId);

    aclError err;
    err = aclrtFree((void *)ptr);
    if (err != ACL_SUCCESS) HICR_THROW_LOGIC("Error while freeing device %d memory. Error %d", deviceId, err);
  }

  /**
   * Backend-internal implementation of the deregisterMemorySlot function
   *
   * \param[in] memorySlot Memory slot to deregister.
   */
  __USED__ inline void deregisterLocalMemorySlotImpl(HiCR::MemorySlot *memorySlot) override
  {
    HICR_THROW_RUNTIME("Not yet implemented for this backend");
  }

  /**
   * Backend-internal implementation of the deregisterGlobalMemorySlotImpl function
   *
   * \param[in] memorySlot Memory slot to deregister.
   */
  __USED__ inline void deregisterGlobalMemorySlotImpl(HiCR::MemorySlot *memorySlot) override
  {
    HICR_THROW_RUNTIME("Not yet implemented for this backend");
  }

  /**
   * Exchanges memory slots among different local instances of HiCR to enable global (remote) communication
   *
   * \param[in] tag Identifies a particular subset of global memory slots
   * \param[in] memorySlots Array of local memory slots to make globally accessible
   */
  __USED__ inline void exchangeGlobalMemorySlotsImpl(const tag_t tag, const std::vector<globalKeyMemorySlotPair_t> &memorySlots) override
  {
    HICR_THROW_RUNTIME("Not yet implemented for this backend");
  }

  /**
   * Backend-internal implementation of the queryMemorySlotUpdates function
   *
   * \param[in] memorySlot Memory slot to query updates for.
   */
  __USED__ inline void queryMemorySlotUpdatesImpl(HiCR::MemorySlot *memorySlot) override
  {
    HICR_THROW_RUNTIME("Not yet implemented for this backend");
  }

  /**
   * Backend-internal memcpy implementation.
   * Restrictions: Only memory copying between Devices in the same thread or between different threads in the same process is supported.
   * Memory copying between Devices in different processes is not supported.
   *
   * \param[in] destination Destination memory slot
   * \param[in] dst_offset Destination offset
   * \param[in] source Source memory slot
   * \param[in] src_offset Source offset
   * \param[in] size size how the data to be copied
   */
  __USED__ inline void memcpyImpl(HiCR::MemorySlot *destination, const size_t dst_offset, HiCR::MemorySlot *source, const size_t src_offset, const size_t size) override
  {
    // Getting up-casted pointer for the execution unit
    auto s = dynamic_cast<MemorySlot *>(source);
    auto d = dynamic_cast<MemorySlot *>(destination);

    // Checking whether the execution unit passed is compatible with this backend
    if (s == NULL) HICR_THROW_LOGIC("The passed memory slot is not supported by this backend\n");
    if (d == NULL) HICR_THROW_LOGIC("The passed memory slot is not supported by this backend\n");

    aclError err;

    // Get source data
    const auto srcPtr = s->getPointer();
    const auto srcDeviceId = s->getDeviceId();
    const auto srcdeviceType = _deviceStatusMap.at(srcDeviceId).deviceType;

    // Get destination data
    const auto dstPtr = d->getPointer();
    const auto dstDeviceId = d->getDeviceId();
    const auto dstdeviceType = _deviceStatusMap.at(dstDeviceId).deviceType;

    // Calculating actual offsets
    const auto actualSrcPtr = (void *)((uint8_t *)srcPtr + src_offset);
    const auto actualDstPtr = (void *)((uint8_t *)dstPtr + dst_offset);

    aclrtMemcpyKind memcpyKind = getMemcpyKind(srcdeviceType, dstdeviceType);

    if (memcpyKind == ACL_MEMCPY_HOST_TO_DEVICE || (memcpyKind == ACL_MEMCPY_DEVICE_TO_DEVICE && dstDeviceId != srcDeviceId))
    {
      selectDevice(dstDeviceId);
    }
    else if (memcpyKind == ACL_MEMCPY_DEVICE_TO_DEVICE || memcpyKind == ACL_MEMCPY_DEVICE_TO_HOST)
    {
      selectDevice(srcDeviceId);
    }

    err = aclrtMemcpy(actualDstPtr, size, actualSrcPtr, size, memcpyKind);

    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not copy memory from device. Error %d", err);

    source->increaseMessagesSent();
    destination->increaseMessagesRecv();
  }

  /**
   * Determine the correct kind of memcpy to be used according to the source and destination device (can be either ascend or host)
   *
   * \param[in] src source device type
   * \param[in] dst destination device type
   *
   * \return the memcpy kind to be used
   */
  __USED__ inline aclrtMemcpyKind getMemcpyKind(deviceType_t src, deviceType_t dst) const
  {
    aclrtMemcpyKind memcpyKind;
    if (src == deviceType_t::Host && dst == deviceType_t::Host)
    {
      memcpyKind = ACL_MEMCPY_HOST_TO_HOST;
    }
    else if (src == deviceType_t::Host && dst == deviceType_t::Npu)
    {
      memcpyKind = ACL_MEMCPY_HOST_TO_DEVICE;
    }
    else if (src == deviceType_t::Npu && dst == deviceType_t::Host)
    {
      memcpyKind = ACL_MEMCPY_DEVICE_TO_HOST;
    }
    else
    {
      memcpyKind = ACL_MEMCPY_DEVICE_TO_DEVICE;
    }

    return memcpyKind;
  }

  /**
   * Backend-internal implementation of the fence function
   *
   * \param[in] tag A tag that releases all processes that share the same value once they have arrived at it
   *
   */
  __USED__ inline void fenceImpl(const tag_t tag) override
  {
  }
};
} // namespace ascend
} // namespace backend
} // namespace HiCR
