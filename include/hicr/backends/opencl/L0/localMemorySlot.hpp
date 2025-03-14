/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file opencl/L0/localMemorySlot.hpp
 * @brief Provides a definition for the local memory slot class for the OpenCL backend
 * @author L. Terracciano
 * @date 06/03/2025
 */
#pragma once

#include <CL/opencl.hpp>
#include <hicr/core/L0/localMemorySlot.hpp>
#include <hicr/core/L0/memorySpace.hpp>

namespace HiCR
{

namespace backend
{

namespace opencl
{

namespace L0
{

/**
 * This class represents an abstract definition for a Local Memory Slot resource for the OpenCL backend
 */
class LocalMemorySlot final : public HiCR::L0::LocalMemorySlot
{
  public:

  /**
   * Constructor for a MemorySlot class for the OpenCL backend
   *
   * \param pointer if this is a local slot (same rank as this the running process), this pointer indicates the address of the local memory segment
   * \param size the size of the memory slot, assumed to be contiguous
   * \param buffer the OpenCL buffer created for the memory slot
   * \param memorySpace the OpenCL memory from which this memory slot was obtained
   */
  LocalMemorySlot(void *const pointer, size_t size, const std::shared_ptr<cl::Buffer> buffer, std::shared_ptr<HiCR::L0::MemorySpace> memorySpace)
    : HiCR::L0::LocalMemorySlot(pointer, size, memorySpace),
      _buffer(buffer){};

  /**
   * Default destructor
   */
  ~LocalMemorySlot() = default;

  /**
   * Get the OpenCL buffer
   * 
   * \return OpenCL buffer
  */
  [[nodiscard]] __INLINE__ std::shared_ptr<cl::Buffer> getBuffer() { return _buffer; }

  private:

  /**
   * The OpenCL buffer associated with the memory slot
   */
  std::shared_ptr<cl::Buffer> _buffer;
};

} // namespace L0

} // namespace opencl

} // namespace backend

} // namespace HiCR
