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

#include <hicr/memorySlot.hpp>
#include <acl/acl.h>

namespace HiCR
{

namespace backend
{

namespace ascend
{

/**
 * Type definition for the Ascend Device identifier
 */
typedef uint64_t deviceIdentifier_t;

/**
 * This class represents an abstract definition for a Memory Slot resource in HiCR that:
 *
 * - Represents a contiguous segment within a memory space, with a starting address, a size, and the Ascend device ID
 */
class MemorySlot final : public HiCR::MemorySlot
{
  public:

  /**
   * Constructor for a MemorySlot class for the MPI backend
   *
   * \param[in] deviceId Ascend device id this memory slot belongs
   * \param[in] pointer If this is a local slot (same rank as this the running process), this pointer indicates the address of the local memory segment
   * \param[in] size The size (in bytes) of the memory slot, assumed to be contiguous
   * \param[in] globalTag For global memory slots, indicates the subset of global memory slots this belongs to
   * \param[in] globalKey Unique identifier for that memory slot that this slot occupies.
   */
  MemorySlot(
    deviceIdentifier_t deviceId,
    void *const pointer,
    size_t size,
    const aclDataBuffer *dataBuffer,
    const tag_t globalTag = 0,
    const globalKey_t globalKey = 0) : HiCR::MemorySlot(pointer, size, globalTag, globalKey), _deviceId(deviceId), _dataBuffer(dataBuffer){};

  /**
   * Default destructor
   */
  ~MemorySlot(){
    (void)aclDestroyDataBuffer(_dataBuffer);
  };

  /**
   * Returns the Ascend device id to which this memory slot belongs
   *
   * \return The Ascend device id to which this memory slot belongs
   */
  __USED__ inline const deviceIdentifier_t getDeviceId() const { return _deviceId; }

  __USED__ inline const aclDataBuffer *getDataBuffer() const { return _dataBuffer; }

  private:

  /**
   * The Ascend Device ID in which the memory slot is created
   */
  const deviceIdentifier_t _deviceId;
  const aclDataBuffer *_dataBuffer;
};
} // namespace ascend
} // namespace backend
} // namespace HiCR
