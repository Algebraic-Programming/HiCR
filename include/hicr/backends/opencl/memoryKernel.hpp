/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file memoryKernel.hpp
 * @brief This file implements the memory kernel class for the opencl backend
 * @author L. Terracciano
 * @date 07/03/2025
 */

#pragma once

#include <CL/opencl.hpp>
#include <hicr/core/exceptions.hpp>
#include <hicr/core/L0/localMemorySlot.hpp>
#include <hicr/backends/opencl/L1/communicationManager.hpp>
#include <hicr/backends/opencl/kernel.hpp>

namespace HiCR
{

namespace backend
{

namespace opencl
{

/**
 * This class represents a replicable Memory Kernel for the opencl backend.
 * A Memory Kernel enable the execution of memcopy operations in a stream/sequence of Kernels.
 * Memory Kernels currently supports memcpy operations on the same device, since they are meant to be chained
 * with other Kernels.
 */
class MemoryKernel final : public Kernel
{
  public:

  /**
   * Constructor for the execution unit class of the opencl backend
   *
   * \param commManager the OpenCL communication manager
   * \param destination destination pointer
   * \param destinationOffset destination offset
   * \param source source pointer
   * \param sourceOffset source offset
   * \param size the number of bytes to copy
   */
  MemoryKernel(opencl::L1::CommunicationManager          *commManager,
               std::shared_ptr<HiCR::L0::LocalMemorySlot> destination,
               const size_t                               destinationOffset,
               std::shared_ptr<HiCR::L0::LocalMemorySlot> source,
               const size_t                               sourceOffset,
               size_t                                     size)
    : opencl::Kernel(),
      _dst(destination),
      _src(source),
      _dstOffset(destinationOffset),
      _srcOffset(sourceOffset),
      _size(size),
      _commManager(commManager){};

  MemoryKernel() = delete;

  /**
   * Default destructor
   */
  ~MemoryKernel() {}

  /**
   * Execute the memcpy on the \p queue
   *
   * \param queue OpenCL queue on which memcpy is executed
   */
  __INLINE__ void start(const cl::CommandQueue *queue) override { _commManager->memcpyAsync(_dst.lock(), _dstOffset, _src.lock(), _srcOffset, _size, queue); }

  private:

  /**
   * Destionation memory slot
   */
  const std::weak_ptr<HiCR::L0::LocalMemorySlot> _dst;

  /**
   * Source memory slot
   */
  const std::weak_ptr<HiCR::L0::LocalMemorySlot> _src;

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
   * OpenCL memory manager
   */
  opencl::L1::CommunicationManager *const _commManager;
};

} // namespace opencl

} // namespace backend

} // namespace HiCR
