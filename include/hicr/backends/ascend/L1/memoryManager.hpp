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

#include <unordered_map>
#include <acl/acl.h>
#include <hicr/L1/memoryManager.hpp>
#include <hicr/backends/ascend/common.hpp>
#include <hicr/backends/ascend/core.hpp>
#include <hicr/backends/ascend/L0/memorySlot.hpp>

namespace HiCR
{

namespace backend
{

namespace ascend
{

namespace L1
{

/**
 * Implementation of the Memory Manager for the Ascend backend.
 *
 * It stores the memory spaces detected by the Ascend computing language
 */
class MemoryManager final : public HiCR::L1::MemoryManager
{
  public:

  /**
   * Constructor for the ascend memory manager class for the ascend backend.
   *
   * \param core ACL Core initializer
   */
  MemoryManager(const Core &core) : HiCR::L1::MemoryManager(), _deviceStatusMap(core.getContexts())
  {
    aclError err;
    aclrtStream stream;
    // create the stream to be used in the memcpy operations
    for (const auto &[deviceId, deviceStatus] : _deviceStatusMap)
    {
      // skip the host device
      if (deviceStatus.deviceType == deviceType_t::Host) continue;

      selectDevice(deviceStatus.context, deviceId);

      err = aclrtCreateStream(&stream);
      if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not create stream on device %d. Error %d", deviceId, err);

      _deviceStreamMap[deviceId] = stream;
    }
  };

  ~MemoryManager()
  {
    aclError err;
    // destroy the stream created in the constructor
    for (const auto &[deviceId, deviceStream] : _deviceStreamMap)
    {
      const auto deviceStatus = _deviceStatusMap.at(deviceId);

      selectDevice(deviceStatus.context, deviceId);

      err = aclrtDestroyStream(deviceStream);
      if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not destroy stream on device %d. Error %d", deviceId, err);
    }
  };

  /**
   * Set the ACL \p stream in which the next memcpy operations needs to be executed.
   *
   * \param stream ACL stream on which future memcpy will be executed
   */
  __USED__ inline void setMemcpyStream(const aclrtStream stream)
  {
    _stream = stream;
  }

  /**
   * Reset the ACL \p stream to its default value.
   *
   */
  __USED__ inline void resetMemcpyStream()
  {
    _stream = NULL;
  }

  /**
   * Get the device id associated with the host system
   *
   * \param memorySpaces the collection of memorySpaces
   *
   * \return the id associated with the host memory
   */
  __USED__ inline const memorySpaceId_t getHostId(const std::set<memorySpaceId_t> memorySpaces)
  {
    for (const auto m : memorySpaces)
    {
      if (_deviceStatusMap.at(m).deviceType == deviceType_t::Host) return m;
    }

    HICR_THROW_RUNTIME("No ID associated with the host system");
  }

  private:

  /**
   * Keep track of the context for each memorySpaceId/deviceId
   */
  const std::unordered_map<memorySpaceId_t, ascendState_t> &_deviceStatusMap;

  std::unordered_map<memorySpaceId_t, aclrtStream> _deviceStreamMap;
  /**
   * Stream on which memcpy operations are executed. The default value is NULL (use the default ACL stream)
   */
  aclrtStream _stream = NULL;

  /**
   * This function returns the available allocatable size (HBM) in the current system RAM
   *
   * \param memorySpace either the memory space representing the device or the host system
   *
   * \return the allocatable size within the system
   */
  __USED__ inline size_t getMemorySpaceSizeImpl(const memorySpaceId_t memorySpace) const override
  {
    const auto deviceState = _deviceStatusMap.at(memorySpace);

    return deviceState.size;
  }

  /**
   * Backend-internal implementation that returns a memory space representing the entire RAM device memory and the
   * other ones representing the Ascends connected to the host.
   *
   * \return a list of memory spaces representing the system status (host memory + ascend devices connected)
   */
  __USED__ inline memorySpaceList_t queryMemorySpacesImpl() override
  {
    // New memory space list to return
    memorySpaceList_t memorySpaceList;

    // Add as many memory spaces as devices
    for (const auto [deviceId, _] : _deviceStatusMap) memorySpaceList.insert(deviceId);

    return memorySpaceList;
  }

  /**
   * Backend-internal implementation of the queryLocalMemorySlot function
   *
   * \param[in] memorySpaceId memory space to allocate memory in
   * \param[in] size size of the memory slot to create
   * \return the internal pointer associated to the local memory slot
   */
  __USED__ inline HiCR::L0::MemorySlot *allocateLocalMemorySlotImpl(const memorySpaceId_t memorySpaceId, const size_t size) override
  {
    void *ptr = NULL;
    aclDataBuffer *dataBuffer;

    // discriminate whether the memory space id reprensets the host or the ascend devices
    if (_deviceStatusMap.at(memorySpaceId).deviceType == deviceType_t::Host)
    {
      // do a malloc on the host and do not create the databuffer
      ptr = hostAlloc(size);
      dataBuffer = NULL;
    }
    else
    {
      // do a malloc on the ascend and create the databuffer
      ptr = deviceAlloc(memorySpaceId, size);
      dataBuffer = aclCreateDataBuffer(ptr, size);
      if (dataBuffer == NULL) HICR_THROW_RUNTIME("Can not create data buffer in memory space %d", memorySpaceId);
    }

    // create the new memory slot
    return new L0::MemorySlot(memorySpaceId, ptr, size, dataBuffer);
  }

  /**
   * Allocate memory on the Host memory through Ascend-dedicated functions.
   *
   * \param size allocation size
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
   * \param memorySpace device id where memory is allocated
   * \param size allocation size
   */
  __USED__ inline void *deviceAlloc(const memorySpaceId_t memorySpace, const size_t size)
  {
    // select the device context on which we should allocate the memory
    selectDevice(_deviceStatusMap.at(memorySpace).context, memorySpace);

    void *ptr;

    // do the allocation on device memory
    aclError err = aclrtMalloc(&ptr, size, ACL_MEM_MALLOC_HUGE_FIRST);
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not allocate memory on ascend device %d. Error %d", memorySpace, err);

    return ptr;
  }
  /**
   * Backend-internal implementation of the registerLocalMemorySlot function. Not implemented.
   *
   * \param ptr pointer to the start of the memory slot
   * \param size size of the memory slot to create
   * \return a newly created memory slot
   */
  __USED__ inline HiCR::L0::MemorySlot *registerLocalMemorySlotImpl(void *const ptr, const size_t size) override
  {
    HICR_THROW_RUNTIME("Not yet implemented for this backend");
  }

  /**
   * Backend-internal implementation of the freeLocalMemorySlot function
   *
   * \param memorySlot local memory slot to free up. It becomes unusable after freeing.
   */
  __USED__ inline void freeLocalMemorySlotImpl(HiCR::L0::MemorySlot *memorySlot) override
  {
    // Getting up-casted pointer for the execution unit
    auto m = dynamic_cast<L0::MemorySlot *>(memorySlot);

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
   * \param ptr local memory to free up.
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
   * \param deviceId device id where memory is allocated.
   * \param ptr local memory to free up.
   */
  __USED__ inline void freeDeviceMemorySlot(const deviceIdentifier_t deviceId, const void *ptr)
  {
    selectDevice(_deviceStatusMap.at(deviceId).context, deviceId);

    aclError err = aclrtFree((void *)ptr);
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Error while freeing device %d memory. Error %d", deviceId, err);
  }

  /**
   * Backend-internal implementation of the deregisterMemorySlot function
   *
   * \param memorySlot memory slot to deregister.
   */
  __USED__ inline void deregisterLocalMemorySlotImpl(HiCR::L0::MemorySlot *memorySlot) override
  {
    HICR_THROW_RUNTIME("Not yet implemented for this backend");
  }

  /**
   * Backend-internal implementation of the deregisterGlobalMemorySlotImpl function
   *
   * \param memorySlot memory slot to deregister.
   */
  __USED__ inline void deregisterGlobalMemorySlotImpl(HiCR::L0::MemorySlot *memorySlot) override
  {
    HICR_THROW_RUNTIME("Not yet implemented for this backend");
  }

  /**
   * Exchanges memory slots among different local instances of HiCR to enable global (remote) communication
   *
   * \param tag identifies a particular subset of global memory slots
   * \param memorySlots array of local memory slots to make globally accessible
   */
  __USED__ inline void exchangeGlobalMemorySlotsImpl(const HiCR::L0::MemorySlot::tag_t tag, const std::vector<globalKeyMemorySlotPair_t> &memorySlots) override
  {
    HICR_THROW_RUNTIME("Not yet implemented for this backend");
  }

  /**
   * Backend-internal implementation of the queryMemorySlotUpdates function
   *
   * \param memorySlot memory slot to query updates for.
   */
  __USED__ inline void queryMemorySlotUpdatesImpl(HiCR::L0::MemorySlot *memorySlot) override
  {
    HICR_THROW_RUNTIME("Not yet implemented for this backend");
  }

  /**
   * Backend-internal memcpy implementation. If the user provides a valid ACL stream via @ref setMemcpyStream() the function will reuse the stream for all the subsequent memcpy invocation on the same device,
   * otherwise it will use one of the ones instantieted by the memory manager. This memcpy implementation does support asynchronous inter-device communication, meaning the fence should be called when date are
   * moved among different ascend devices.
   *
   * Restrictions:
   * - Only memory copying between devices in the same thread or between different threads in the same process is supported. Memory copying between Devices in different processes is not supported.
   *
   * \param destination destination memory slot
   * \param dst_offset destination offset
   * \param source source memory slot
   * \param src_offset source offset
   * \param size the number of bytes to copy
   */
  __USED__ inline void memcpyImpl(HiCR::L0::MemorySlot *destination, const size_t dst_offset, HiCR::L0::MemorySlot *source, const size_t src_offset, const size_t size) override
  {
    // Getting up-casted pointer for the execution unit
    auto s = dynamic_cast<L0::MemorySlot *>(source);
    auto d = dynamic_cast<L0::MemorySlot *>(destination);

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

    // determine the correct memcpy kind
    aclrtMemcpyKind memcpyKind = getMemcpyKind(srcdeviceType, dstdeviceType);

    L0::MemorySlot *deviceMemSlot;

    // select device according to memcpy kind
    if (memcpyKind == ACL_MEMCPY_HOST_TO_DEVICE || (memcpyKind == ACL_MEMCPY_DEVICE_TO_DEVICE && dstDeviceId != srcDeviceId))
    {
      deviceMemSlot = d;
    }
    else if (memcpyKind == ACL_MEMCPY_DEVICE_TO_DEVICE || memcpyKind == ACL_MEMCPY_DEVICE_TO_HOST)
    {
      deviceMemSlot = s;
    }
    else
    {
      // async memcpy not supported between host to host case
      err = aclrtMemcpy(actualDstPtr, size, actualSrcPtr, size, memcpyKind);
      if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not perform memcpy of type %d in host. Error %d", memcpyKind, err);
      s->increaseMessagesSent();
      d->increaseMessagesRecv();
      return;
    }

    // if no stream has already been provided, there are no pending operations to execute. In this case, use one of the streams already created by the memory manager
    bool streamModeEnabled = _stream != NULL;
    if (!streamModeEnabled)
    {
      const auto deviceId = deviceMemSlot->getDeviceId();
      const auto deviceStatus = _deviceStatusMap.at(deviceId);
      selectDevice(deviceStatus.context, deviceId);
      // get one of the already created streams
      _stream = _deviceStreamMap.at(deviceId);
    }

    // execute memcpy
    err = aclrtMemcpyAsync(actualDstPtr, size, actualSrcPtr, size, memcpyKind, _stream);
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not perform memcpy of type %d from device %d to device %d. Error %d", memcpyKind, srcDeviceId, dstDeviceId, err);

    // increase message counters and keep track of streams in memory slot
    s->increaseMessagesSent();
    d->increaseMessagesRecv();
  }

  /**
   * Determine the correct kind of memcpy to be used according to the source and destination device (can be either ascend or host)
   *
   * \param src source device type
   * \param dst destination device type
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
   * Backend-internal implementation of the fence function.
   *
   * \param tag A tag that releases all processes that share the same value once they have arrived at it
   *
   */
  __USED__ inline void fenceImpl(const HiCR::L0::MemorySlot::tag_t tag) override
  {
    // no need to fence if stream not set
    if (_stream == NULL) return;
    // no need to do context setting
    aclError err = aclrtSynchronizeStream(_stream);
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Failed to destroy stream. Error %d", err);
    _stream = NULL;
  }

  __USED__ inline bool acquireGlobalLockImpl(HiCR::L0::MemorySlot *memorySlot) override
  {
    HICR_THROW_RUNTIME("Not yet implemented for this backend");
  }

  __USED__ inline void releaseGlobalLockImpl(HiCR::L0::MemorySlot *memorySlot) override
  {
    HICR_THROW_RUNTIME("Not yet implemented for this backend");
  }
};

} // namespace L1

} // namespace ascend

} // namespace backend

} // namespace HiCR
