/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file memoryKernel.hpp
 * @brief This file implements the kernel class for the ascend backend
 * @author S. M. Martin & L. Terracciano
 * @date 13/11/2023
 */

#pragma once

#include <acl/acl.h>
#include <hicr/backends/ascend/kernel.hpp>
#include <hicr/backends/ascend/memoryManager.hpp>
#include <hicr/common/exceptions.hpp>
#include <hicr/memorySlot.hpp>

namespace HiCR
{

namespace backend
{

namespace ascend
{

namespace kernel
{

/**
 * This class represents a replicable Memory Kernel for the ascend backend.
 * A Memory Kernel enable the execution of memcopy operations in a stream/sequence of Kernels.
 * Memory Kernels currently supports memcpy operations on the same device, since they are meant to be chained
 * with other Kernels.
 */
class MemoryKernel final : public Kernel
{
  public:

  /**
   * Constructor for the execution unit class of the ascend backend
   *
   * \param memManager the Ascend memory manager
   * \param destination destination pointer
   * \param destinationOffset destination offset
   * \param source source pointer
   * \param sourceOffset source offset
   * \param size the number of bytes to copy
   */
  MemoryKernel(MemoryManager *memManager, HiCR::MemorySlot *destination, const size_t destinationOffset, HiCR::MemorySlot *source, const size_t sourceOffset, size_t size) : Kernel(),
                                                                                                                                                                             _dst(destination),
                                                                                                                                                                             _src(source),
                                                                                                                                                                             _dstOffset(destinationOffset),
                                                                                                                                                                             _srcOffset(sourceOffset),
                                                                                                                                                                             _size(size),
                                                                                                                                                                             _memManager(memManager){};
  MemoryKernel() = delete;

  /**
   * Default destructor
   */
  ~MemoryKernel() {}

  /**
   * Execute the memcpy on the \p stream
   *
   * \param stream ACL stream on which memcpy is executed
   */
  __USED__ inline void start(const aclrtStream stream) override
  {
    _memManager->setMemcpyStream(stream);
    _memManager->memcpy(_dst, _dstOffset, _src, _srcOffset, _size);
  }

  private:

  /**
   * Destionation memory slot
   */
  HiCR::MemorySlot *_dst;
  /**
   * Source memory slot
   */
  HiCR::MemorySlot *_src;

  /**
   * Destination offset
   */
  const size_t _dstOffset;

  /**
   * Source offset
   */
  const size_t _srcOffset;
  /**
   * Data size to be copied
   */
  const size_t _size;

  /**
   * Ascend memory manager
   */
  MemoryManager *_memManager;
};

} // namespace kernel
} // namespace ascend
} // namespace backend
} // namespace HiCR
