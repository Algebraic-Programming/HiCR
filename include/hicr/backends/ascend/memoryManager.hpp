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
#include <hicr/backends/ascend/common.hpp>
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
 * Implementation of the memory manager for the Ascend backend
 */
class MemoryManager final : public backend::MemoryManager
{
  public:

  /**
   * Constructor for the ascend memory manager class for the ascend backend.
   */
  MemoryManager() : HiCR::backend::MemoryManager(){

                    };

  ~MemoryManager() = default;

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
  std::map<memorySpaceId_t, ascendState_t> _deviceStatusMap;

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
    // Clearing existing memory space map
    _deviceStatusMap.clear();

    // New memory space list to return
    memorySpaceList_t memorySpaceList;

    // Add as many memory spaces as devices
    for (const auto deviceData : _deviceStatusMap) memorySpaceList.insert(deviceData.first);

    // Add host memory space
    memorySpaceList.insert(_deviceCount);

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
    aclDataBuffer *dataBuffer;
    if (_deviceStatusMap.at(memorySpaceId).deviceType == deviceType_t::Host)
    {
      ptr = hostAlloc(size);
      dataBuffer = NULL;
    }
    else
    {
      ptr = deviceAlloc(memorySpaceId, size);
      dataBuffer = aclCreateDataBuffer(ptr, size);
      if (dataBuffer == NULL) HICR_THROW_RUNTIME("Can not create data buffer in memory space %d", memorySpaceId);
    }

    // keep track of the mapping between unique identifier and pointer + deviceId
    auto memorySlot = new MemorySlot(memorySpaceId, ptr, size, dataBuffer);

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

    aclError err;

    // select the device context on which we should allocate the memory
    err = selectDevice(_deviceStatusMap.at(memorySpace).context);
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not select the device %ld. Error %d", memorySpace, err);

    // do the allocation on device memory
    err = aclrtMalloc(&ptr, size, ACL_MEM_MALLOC_HUGE_FIRST);
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
      aclError err = aclDestroyDataBuffer(m->getDataBuffer());
      if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not destroy data buffer. Error %d", err);
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
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Error while freeing host memory. Error %d", err);
  }

  /**
   * Release memory on the Ascend memory through Ascend-dedicated functions.
   *
   * \param[in] deviceId Device id where memory is allocated.
   * \param[in] ptr Local memory to free up.
   */
  __USED__ inline void freeDeviceMemorySlot(const deviceIdentifier_t deviceId, const void *ptr)
  {
    aclError err;

    err = selectDevice(_deviceStatusMap.at(deviceId).context);
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not select the device %ld. Error %d", deviceId, err);

    err = aclrtFree((void *)ptr);
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Error while freeing device %d memory. Error %d", deviceId, err);
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
      err = selectDevice(_deviceStatusMap.at(dstDeviceId).context);
      if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not select the device %ld. Error %d", dstDeviceId, err);
    }
    else if (memcpyKind == ACL_MEMCPY_DEVICE_TO_DEVICE || memcpyKind == ACL_MEMCPY_DEVICE_TO_HOST)
    {
      err = selectDevice(_deviceStatusMap.at(srcDeviceId).context);
      if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not select the device %ld. Error %d", srcDeviceId, err);
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
