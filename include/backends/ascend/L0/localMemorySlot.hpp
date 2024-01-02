/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file ascend/L0/localMemorySlot.hpp
 * @brief Provides a definition for the local memory slot class for the Ascend backend
 * @author L. Terracciano & S. M. Martin
 * @date 19/10/2023
 */
#pragma once

#include <acl/acl.h>
#include <hicr/L0/localMemorySlot.hpp>
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
 * This class represents an abstract definition for a Local Memory Slot resource for the Ascend backend
 */
class LocalMemorySlot final : public HiCR::L0::LocalMemorySlot
{
  public:

  /**
   * Constructor for a MemorySlot class for the Ascend backend
   *
   * \param pointer if this is a local slot (same rank as this the running process), this pointer indicates the address of the local memory segment
   * \param size the size of the memory slot, assumed to be contiguous
   * \param dataBuffer the ACL data buffer created for the memory slot
   * \param memorySpace the Ascend memory from which this memory slot was obtained
   */
  LocalMemorySlot(
    void *const pointer,
    size_t size,
    const aclDataBuffer *dataBuffer,
    HiCR::L0::MemorySpace* memorySpace) : HiCR::L0::LocalMemorySlot(pointer, size, memorySpace), _dataBuffer(dataBuffer){};

  /**
   * Default destructor
   */
  ~LocalMemorySlot() = default;

  /**
   * Return the ACL data buffer associated to the memory slot
   *
   * \return the ACL data buffer associated to the memory slot
   */
  __USED__ inline const aclDataBuffer *getDataBuffer() const { return _dataBuffer; }

  private:

  /**
   * The Ascend Data Buffer associated with the memory slot
   */
  const aclDataBuffer *_dataBuffer;
};

} // namespace L0

} // namespace ascend

} // namespace backend

} // namespace HiCR
