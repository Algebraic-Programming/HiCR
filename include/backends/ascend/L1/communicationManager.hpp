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

  /**
   * Enumeration to indicate a type of device involved in data communication operations
   */
  enum deviceType_t
  {
    /**
     * No device -- used as safeguard to detect errors
     */
    none,

    /**
     * Host -- Involves the main host memory (RAM) in the operation
     */
    host,

    /**
     * Device -- Involves an Ascend device memory (DRAM) in the operation
     */
    device
  };

  /**
   * Constructor for the ascend communication manager class for the ascend backend.
   */
  CommunicationManager() : HiCR::L1::CommunicationManager(){};
  ~CommunicationManager() = default;

  /**
   * Backend-internal asyncrhonous implementation of the memcpy operation. It passes an Ascend stream as context for later asynchrounous check for completion
   *
   * For more information, see: memcpyImpl
   *
   * \param[in] destination destination memory slot
   * \param[in] dst_offset destination offset
   * \param[in] source source memory slot
   * \param[in] src_offset source offset
   * \param[in] size the number of bytes to copy
   * \param[in] stream Ascend stream containing the state of the operation for later check
   */
  __USED__ inline void memcpyAsync(std::shared_ptr<HiCR::L0::LocalMemorySlot> destination, const size_t dst_offset, std::shared_ptr<HiCR::L0::LocalMemorySlot> source, const size_t src_offset, const size_t size, const aclrtStream stream)
  {
    memcpyInternal(destination, dst_offset, source, src_offset, size, stream);
  }

  private:

  __USED__ inline void deregisterGlobalMemorySlotImpl(const std::shared_ptr<HiCR::L0::GlobalMemorySlot> memorySlot) override
  {
    HICR_THROW_RUNTIME("Not yet implemented for this backend");
  }

  __USED__ inline void exchangeGlobalMemorySlotsImpl(const HiCR::L0::GlobalMemorySlot::tag_t tag, const std::vector<globalKeyMemorySlotPair_t> &memorySlots) override
  {
    HICR_THROW_RUNTIME("Not yet implemented for this backend");
  }

  __USED__ inline void queryMemorySlotUpdatesImpl(std::shared_ptr<HiCR::L0::GlobalMemorySlot> memorySlot) override
  {
    HICR_THROW_RUNTIME("Not yet implemented for this backend");
  }

  /**
   * This memcpy implementation does support asynchronous inter-device communication, meaning the fence should be called when date are
   * moved among different ascend devices.
   *
   * Restrictions:
   * - Only memory copying between devices in the same thread or between different threads in the same process is supported. Memory copying between Devices in different processes is not supported.
   *
   * \param[in] destination destination memory slot
   * \param[in] dst_offset destination offset
   * \param[in] source source memory slot
   * \param[in] src_offset source offset
   * \param[in] size the number of bytes to copy
   */
  __USED__ inline void memcpyImpl(std::shared_ptr<HiCR::L0::LocalMemorySlot> destination, const size_t dst_offset, std::shared_ptr<HiCR::L0::LocalMemorySlot> source, const size_t src_offset, const size_t size) override
  {
    memcpyInternal(destination, dst_offset, source, src_offset, size, NULL);
  }

  __USED__ inline void memcpyInternal(std::shared_ptr<HiCR::L0::LocalMemorySlot> destination, const size_t dst_offset, std::shared_ptr<HiCR::L0::LocalMemorySlot> source, const size_t src_offset, const size_t size, const aclrtStream stream)
  {
    // Storage for device type
    deviceType_t srcType = deviceType_t::none;
    deviceType_t dstType = deviceType_t::none;

    // Using up-casting to determine device types
    auto sd = dynamic_pointer_cast<ascend::L0::LocalMemorySlot>(source);
    auto dd = dynamic_pointer_cast<ascend::L0::LocalMemorySlot>(destination);
    auto sh = dynamic_pointer_cast<HiCR::L0::LocalMemorySlot>(source);
    auto dh = dynamic_pointer_cast<HiCR::L0::LocalMemorySlot>(destination);

    if (sh != NULL) srcType = deviceType_t::host;
    if (dh != NULL) dstType = deviceType_t::host;
    if (sd != NULL) srcType = deviceType_t::device;
    if (dd != NULL) dstType = deviceType_t::device;

    // Checking whether the memory slot unit passed is compatible with this backend
    if (srcType == deviceType_t::none) HICR_THROW_LOGIC("The passed source memory slot is not supported by this backend\n");
    if (dstType == deviceType_t::none) HICR_THROW_LOGIC("The passed destination memory slot is not supported by this backend\n");

    // Storage for pointers and memcpy kind
    aclrtMemcpyKind memcpyKind;
    auto            srcPtr = source->getPointer();
    auto            dstPtr = destination->getPointer();

    // Determining which device context to use for copying
    std::shared_ptr<ascend::L0::LocalMemorySlot> deviceMemSlot = NULL;
    if (srcType == deviceType_t::host && dstType == deviceType_t::host)
    {
      deviceMemSlot = NULL;
      memcpyKind    = ACL_MEMCPY_HOST_TO_HOST;
    }
    if (srcType == deviceType_t::host && dstType == deviceType_t::device)
    {
      deviceMemSlot = dd;
      memcpyKind    = ACL_MEMCPY_HOST_TO_DEVICE;
    }
    if (srcType == deviceType_t::device && dstType == deviceType_t::host)
    {
      deviceMemSlot = sd;
      memcpyKind    = ACL_MEMCPY_DEVICE_TO_HOST;
    }
    if (srcType == deviceType_t::device && dstType == deviceType_t::device)
    {
      deviceMemSlot = dd;
      memcpyKind    = ACL_MEMCPY_DEVICE_TO_DEVICE;
    }

    // Calculating actual offsets
    const auto actualSrcPtr = (uint8_t *)srcPtr + src_offset;
    const auto actualDstPtr = (uint8_t *)dstPtr + dst_offset;

    // If a device is involved in this operation, select it and use its stream to perform the operation
    if (deviceMemSlot != NULL) dynamic_pointer_cast<ascend::L0::MemorySpace>(deviceMemSlot->getMemorySpace())->getDevice().lock()->select();

    // Now executing memcpy depending on whether a stream was specified
    aclError err;
    if (stream != NULL) err = aclrtMemcpyAsync(actualDstPtr, size, actualSrcPtr, size, memcpyKind, stream);
    if (stream == NULL) err = aclrtMemcpy(actualDstPtr, size, actualSrcPtr, size, memcpyKind);

    // Check for errors
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Could not perform memcpy of type %d. Error %d", memcpyKind, err);

    // Increasing message received/sent counters for both memory slots
    destination->increaseMessagesRecv();
    source->increaseMessagesSent();
  }

  __USED__ inline void fenceImpl(const HiCR::L0::GlobalMemorySlot::tag_t tag) override
  {
    // Not yet implemented
  }

  __USED__ inline bool acquireGlobalLockImpl(std::shared_ptr<HiCR::L0::GlobalMemorySlot> memorySlot) override
  {
    HICR_THROW_RUNTIME("Not yet implemented for this backend");
  }

  __USED__ inline void releaseGlobalLockImpl(std::shared_ptr<HiCR::L0::GlobalMemorySlot> memorySlot) override
  {
    HICR_THROW_RUNTIME("Not yet implemented for this backend");
  }
};

} // namespace L1

} // namespace ascend

} // namespace backend

} // namespace HiCR
