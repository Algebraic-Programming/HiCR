/*
 *   Copyright 2025 Huawei Technologies Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file communicationManager.hpp
 * @brief This file implements the communication manager class for the Ascend backend
 * @author L. Terracciano & S. M. Martin
 * @date 21/12/2023
 */

#pragma once

#include <acl/acl.h>
#include <hicr/backends/ascend/memorySpace.hpp>
#include <hicr/backends/ascend/device.hpp>
#include <hicr/backends/ascend/localMemorySlot.hpp>
#include <hicr/core/communicationManager.hpp>
#include <hicr/core/globalMemorySlot.hpp>

namespace HiCR::backend::ascend
{

/**
 * Implementation of the Communication Manager for the Ascend backend.
 
 * \note Supported local memory slots:
 * - Ascend
 * - HWLoC
 */
class CommunicationManager final : public HiCR::CommunicationManager
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
   * Constructor for the ascend communication manager class for the Ascend backend.
   */
  CommunicationManager()
    : HiCR::CommunicationManager(){};
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
   * \param[in] stream Ascend stream containing the state of the operation for later checks for completion
   */
  __INLINE__ void memcpyAsync(const std::shared_ptr<HiCR::LocalMemorySlot> &destination,
                              const size_t                                  dst_offset,
                              const std::shared_ptr<HiCR::LocalMemorySlot> &source,
                              const size_t                                  src_offset,
                              const size_t                                  size,
                              const aclrtStream                             stream)
  {
    memcpyInternal(destination, dst_offset, source, src_offset, size, stream);
  }

  private:

  __INLINE__ void exchangeGlobalMemorySlotsImpl(const HiCR::GlobalMemorySlot::tag_t tag, const std::vector<globalKeyMemorySlotPair_t> &memorySlots) override
  {
    HICR_THROW_RUNTIME("Not yet implemented for this backend");
  }

  __INLINE__ void queryMemorySlotUpdatesImpl(std::shared_ptr<HiCR::LocalMemorySlot> memorySlot) override { HICR_THROW_RUNTIME("Not yet implemented for this backend"); }

  __INLINE__ std::shared_ptr<HiCR::GlobalMemorySlot> getGlobalMemorySlotImpl(const HiCR::GlobalMemorySlot::tag_t tag, const HiCR::GlobalMemorySlot::globalKey_t globalKey) override
  {
    return nullptr;
  }

  /**
   * Deletes a global memory slot from the backend.
   * Attempting to access the global memory slot after this operation will result in undefined behavior.
   *
   * \param[in] memorySlot Memory slot to destroy.
   */
  __INLINE__ void destroyGlobalMemorySlotImpl(const std::shared_ptr<HiCR::GlobalMemorySlot> memorySlot) override { HICR_THROW_RUNTIME("Not yet implemented for this backend"); }

  /**
   * Implementation for memcpy operation
   *
   * \param[in] destination destination memory slot
   * \param[in] dst_offset destination offset
   * \param[in] source source memory slot
   * \param[in] src_offset source offset
   * \param[in] size the number of bytes to copy
   */
  __INLINE__ void memcpyImpl(const std::shared_ptr<HiCR::LocalMemorySlot> &destination,
                             const size_t                                  dst_offset,
                             const std::shared_ptr<HiCR::LocalMemorySlot> &source,
                             const size_t                                  src_offset,
                             const size_t                                  size) override
  {
    memcpyInternal(destination, dst_offset, source, src_offset, size, NULL);
  }

  /**
   * Implementation for sync and async memcpy operation
   *
   * Restrictions:
   * - Only memory copying between devices in the same thread or between different threads in the same process is supported. 
   *   Memory copying between Devices in different processes is not supported.
   *
   * \param[in] destination destination memory slot
   * \param[in] dst_offset destination offset
   * \param[in] source source memory slot
   * \param[in] src_offset source offset
   * \param[in] size the number of bytes to copy
   * \param[in] stream ACL stream. Triggers sync or async behavior if the passed value is NULL or not, respectively
   */
  __INLINE__ void memcpyInternal(const std::shared_ptr<HiCR::LocalMemorySlot> &destination,
                                 const size_t                                  dst_offset,
                                 const std::shared_ptr<HiCR::LocalMemorySlot> &source,
                                 const size_t                                  src_offset,
                                 const size_t                                  size,
                                 const aclrtStream                             stream)
  {
    // Storage for device type
    deviceType_t srcType = deviceType_t::none;
    deviceType_t dstType = deviceType_t::none;

    // Using up-casting to determine device types
    auto sd = dynamic_pointer_cast<ascend::LocalMemorySlot>(source);
    auto dd = dynamic_pointer_cast<ascend::LocalMemorySlot>(destination);
    auto sh = dynamic_pointer_cast<HiCR::LocalMemorySlot>(source);
    auto dh = dynamic_pointer_cast<HiCR::LocalMemorySlot>(destination);

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
    std::shared_ptr<ascend::LocalMemorySlot> deviceMemSlot = NULL;
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
    if (deviceMemSlot != NULL) dynamic_pointer_cast<ascend::MemorySpace>(deviceMemSlot->getMemorySpace())->getDevice().lock()->select();

    // Now executing memcpy depending on whether a stream was specified
    aclError err;
    if (stream != NULL) err = aclrtMemcpyAsync(actualDstPtr, size, actualSrcPtr, size, memcpyKind, stream);
    if (stream == NULL) err = aclrtMemcpy(actualDstPtr, size, actualSrcPtr, size, memcpyKind);

    // Check for errors
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Could not perform memcpy of type %d. Error %d", memcpyKind, err);

    // Increasing message received/sent counters for both memory slots
    increaseMessageRecvCounter(*destination);
    increaseMessageSentCounter(*source);
  }

  __INLINE__ void fenceImpl(const HiCR::GlobalMemorySlot::tag_t tag) override
  {
    // Not yet implemented
  }

  __INLINE__ bool acquireGlobalLockImpl(std::shared_ptr<HiCR::GlobalMemorySlot> memorySlot) override { HICR_THROW_RUNTIME("Not yet implemented for this backend"); }

  __INLINE__ void releaseGlobalLockImpl(std::shared_ptr<HiCR::GlobalMemorySlot> memorySlot) override { HICR_THROW_RUNTIME("Not yet implemented for this backend"); }
};

} // namespace HiCR::backend::ascend
