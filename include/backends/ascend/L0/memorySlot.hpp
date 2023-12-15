/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file memorySlot.hpp
 * @brief Provides a definition for the memory slot class for the Ascend backend
 * @author S. M. Martin and L.Terracciano
 * @date 19/10/2023
 */
#pragma once

#include <acl/acl.h>
#include <backends/ascend/common.hpp>
#include <hicr/L0/memorySlot.hpp>
#include <hicr/L0/memorySpace.hpp>

namespace HiCR
{

namespace backend
{

namespace ascend
{

namespace L0
{

/**
 * This class represents an abstract definition for a Memory Slot resource in HiCR that:
 *
 * Represents a contiguous segment within a memory space, with a starting address, a size, and the Ascend device id
 */
class MemorySlot final : public HiCR::L0::MemorySlot
{
  public:

  /**
   * Constructor for a MemorySlot class for the Ascend backend
   *
   * \param deviceId ascend device id this memory slot belongs
   * \param pointer if this is a local slot (same rank as this the running process), this pointer indicates the address of the local memory segment
   * \param size the size of the memory slot, assumed to be contiguous
   * \param dataBuffer the ACL data buffer created for the memory slot
   * \param globalTag for global memory slots, indicates the subset of global memory slots this belongs to
   * \param globalKey unique identifier for that memory slot that this slot occupies.
   */
  MemorySlot(
    deviceIdentifier_t deviceId,
    void *const pointer,
    size_t size,
    const aclDataBuffer *dataBuffer,
    HiCR::L0::MemorySpace* memorySpace,
    const tag_t globalTag = 0,
    const globalKey_t globalKey = 0) : HiCR::L0::MemorySlot(pointer, size, memorySpace, globalTag, globalKey), _deviceId(deviceId), _dataBuffer(dataBuffer){};

  /**
   * Default destructor
   */
  ~MemorySlot() = default;

  /**
   * Return the Ascend device id to which this memory slot belongs
   *
   * \return the Ascend device id to which this memory slot belongs
   */
  __USED__ inline const deviceIdentifier_t getDeviceId() const { return _deviceId; }

  /**
   * Return the ACL data buffer associated to the memory slot
   *
   * \return the ACL data buffer associated to the memory slot
   */
  __USED__ inline const aclDataBuffer *getDataBuffer() const { return _dataBuffer; }

  private:

  /**
   * The Ascend Device ID in which the memory slot is created
   */
  const deviceIdentifier_t _deviceId;

  /**
   * The Ascend Data Buffer associated with the memory slot
   */
  const aclDataBuffer *_dataBuffer;
};

} // namespace L0

} // namespace ascend

} // namespace backend

} // namespace HiCR
