/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file memoryManager.hpp
 * @brief This file implements the communication manager class for the Ascend backend
 * @author L. Terracciano & S. M. Martin
 * @date 21/12/2023
 */

#pragma once

#include <acl/acl.h>
#include <backends/ascend/L0/memorySpace.hpp>
#include <backends/ascend/L0/device.hpp>
#include <backends/ascend/L0/localMemorySlot.hpp>
#include <backends/ascend/common.hpp>
#include <backends/ascend/core.hpp>
#include <hicr/L1/communicationManager.hpp>
#include <hicr/L0/globalMemorySlot.hpp>

namespace HiCR
{

namespace backend
{

namespace ascend
{

namespace L1
{

/**
 * Implementation of the Communication Manager for the Ascend backend.
 */
class CommunicationManager final : public HiCR::L1::CommunicationManager
{
  public:

  enum deviceType_t { none, host, device };

  /**
   * Constructor for the ascend communication manager class for the ascend backend.
   */
  CommunicationManager() : HiCR::L1::CommunicationManager() {};
  ~CommunicationManager() = default;

  /**
   * Set the ACL \p stream in which the next memcpy operations needs to be executed.
   *
   * \param stream ACL stream on which future memcpy will be executed
   */
  __USED__ inline void setMemcpyStream(const aclrtStream stream) {  _stream = stream; }

  /**
   * Reset the ACL \p stream to its default value.
   */
  __USED__ inline void resetMemcpyStream() { _stream = NULL;  }

  __USED__ inline void memcpyAsync(HiCR::L0::LocalMemorySlot *destination, const size_t dst_offset, HiCR::L0::LocalMemorySlot *source, const size_t src_offset, const size_t size, const aclrtStream stream)
  {
    memcpyInternal(destination,  dst_offset, source, src_offset, size, stream);
  }

  private:

  /**
   * Stream on which memcpy operations are executed. The default value is NULL (use the default ACL stream)
   */
  aclrtStream _stream = NULL;

  /**
   * Backend-internal implementation of the deregisterGlobalMemorySlotImpl function
   *
   * \param memorySlot memory slot to deregister.
   */
  __USED__ inline void deregisterGlobalMemorySlotImpl(HiCR::L0::GlobalMemorySlot *memorySlot) override
  {
    HICR_THROW_RUNTIME("Not yet implemented for this backend");
  }

  /**
   * Exchanges memory slots among different local instances of HiCR to enable global (remote) communication
   *
   * \param tag identifies a particular subset of global memory slots
   * \param memorySlots array of local memory slots to make globally accessible
   */
  __USED__ inline void exchangeGlobalMemorySlotsImpl(const HiCR::L0::GlobalMemorySlot::tag_t tag, const std::vector<globalKeyMemorySlotPair_t> &memorySlots) override
  {
    HICR_THROW_RUNTIME("Not yet implemented for this backend");
  }

  /**
   * Backend-internal implementation of the queryMemorySlotUpdates function
   *
   * \param memorySlot memory slot to query updates for.
   */
  __USED__ inline void queryMemorySlotUpdatesImpl(HiCR::L0::GlobalMemorySlot *memorySlot) override
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
  __USED__ inline void memcpyImpl(HiCR::L0::LocalMemorySlot *destination, const size_t dst_offset, HiCR::L0::LocalMemorySlot *source, const size_t src_offset, const size_t size) override
  {
    memcpyInternal(destination,  dst_offset, source, src_offset, size, NULL);
  }

  __USED__ inline void memcpyInternal(HiCR::L0::LocalMemorySlot *destination, const size_t dst_offset, HiCR::L0::LocalMemorySlot *source, const size_t src_offset, const size_t size, const aclrtStream stream)
  {
    // Storage for device type
    deviceType_t srcType = deviceType_t::none;
    deviceType_t dstType = deviceType_t::none;

    // Using up-casting to determine device types
    auto sd = dynamic_cast<ascend::L0::LocalMemorySlot *>(source);
    auto dd = dynamic_cast<ascend::L0::LocalMemorySlot *>(destination);
    auto sh = dynamic_cast<HiCR::L0::LocalMemorySlot *>(source);
    auto dh = dynamic_cast<HiCR::L0::LocalMemorySlot *>(destination);

    if (sd != NULL) srcType = deviceType_t::device;
    if (dd != NULL) dstType = deviceType_t::device;
    if (sh != NULL) srcType = deviceType_t::host;
    if (dh != NULL) dstType = deviceType_t::host;

    // Checking whether the execution unit passed is compatible with this backend
    if (srcType == deviceType_t::none) HICR_THROW_LOGIC("The passed source memory slot is not supported by this backend\n");
    if (dstType == deviceType_t::none) HICR_THROW_LOGIC("The passed destination memory slot is not supported by this backend\n");

    // Determining memcpy kind
    aclrtMemcpyKind memcpyKind;
    if (srcType == deviceType_t::host   && dstType == deviceType_t::host)   memcpyKind = ACL_MEMCPY_HOST_TO_HOST;
    if (srcType == deviceType_t::host   && dstType == deviceType_t::device) memcpyKind = ACL_MEMCPY_HOST_TO_DEVICE;
    if (srcType == deviceType_t::device && dstType == deviceType_t::host)   memcpyKind = ACL_MEMCPY_DEVICE_TO_HOST;
    if (srcType == deviceType_t::device && dstType == deviceType_t::device) memcpyKind = ACL_MEMCPY_DEVICE_TO_DEVICE;

    // Storage for pointers
    const void* srcPtr = NULL;
    void* dstPtr       = NULL;

    // Determining which device context to use for copying
    ascend::L0::LocalMemorySlot *deviceMemSlot = NULL;
    if (ACL_MEMCPY_HOST_TO_HOST)     { deviceMemSlot = NULL; srcPtr = sh->getPointer(); dstPtr = dh->getPointer(); }
    if (ACL_MEMCPY_HOST_TO_DEVICE)   { deviceMemSlot = dd;   srcPtr = sh->getPointer(); dstPtr = dd->getPointer(); }
    if (ACL_MEMCPY_DEVICE_TO_HOST)   { deviceMemSlot = sd;   srcPtr = sd->getPointer(); dstPtr = dh->getPointer(); }
    if (ACL_MEMCPY_DEVICE_TO_DEVICE) { deviceMemSlot = dd;   srcPtr = sd->getPointer(); dstPtr = dd->getPointer(); }

    // Calculating actual offsets
    const auto actualSrcPtr = (const void *)((uint8_t *)srcPtr + src_offset);
    const auto actualDstPtr = (void *)((uint8_t *)dstPtr + dst_offset);

    // If a device is involved in this operation, select it and use its stream to perform the operation
    if (deviceMemSlot != NULL)
    {
      // Getting memory slot info
      const auto memorySlotMemorySpace = (ascend::L0::MemorySpace *) deviceMemSlot->getMemorySpace();
      const auto memorySlotDevice = memorySlotMemorySpace->getDevice();
 
      // Selecting device
      memorySlotDevice->select();
    } 

    // Now executing memcpy depending on whether a stream was specified
    aclError err;
    if (stream != NULL) err = aclrtMemcpyAsync(actualDstPtr, size, actualSrcPtr, size, memcpyKind, stream);
    if (stream == NULL) err = aclrtMemcpy(actualDstPtr, size, actualSrcPtr, size, memcpyKind);

    // Check for errors
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Could not perform memcpy of type %d. Error %d", memcpyKind, err);
  }


  /**
   * Backend-internal implementation of the fence function.
   *
   * \param tag A tag that releases all processes that share the same value once they have arrived at it
   *
   */
  __USED__ inline void fenceImpl(const HiCR::L0::GlobalMemorySlot::tag_t tag) override
  {
    // no need to fence if stream not set
    if (_stream == NULL) return;

    // no need to do context setting
    aclError err = aclrtSynchronizeStream(_stream);
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Failed to destroy stream. Error %d", err);
    _stream = NULL;
  }

  __USED__ inline bool acquireGlobalLockImpl(HiCR::L0::GlobalMemorySlot *memorySlot) override
  {
    HICR_THROW_RUNTIME("Not yet implemented for this backend");
  }

  __USED__ inline void releaseGlobalLockImpl(HiCR::L0::GlobalMemorySlot *memorySlot) override
  {
    HICR_THROW_RUNTIME("Not yet implemented for this backend");
  }
};

} // namespace L1

} // namespace ascend

} // namespace backend

} // namespace HiCR
