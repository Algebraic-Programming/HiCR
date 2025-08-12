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

/**
 * @file memoryManager.hpp
 * @brief This file implements the memory manager class for the OpenCL backend
 * @author L. Terracciano
 * @date 06/03/2025
 */

#pragma once

#include <CL/opencl.hpp>
#include <hicr/backends/opencl/device.hpp>
#include <hicr/backends/opencl/memorySpace.hpp>
#include <hicr/backends/opencl/localMemorySlot.hpp>
#include <hicr/backends/hwloc/memorySpace.hpp>
#include <hicr/core/memoryManager.hpp>

namespace HiCR::backend::opencl
{
/**
 * Implementation of the Memory Manager for the OpenCL backend.
 *
 * \note Supported memory spaces:
 * - OpenCL
 * - HWLoC
 */
class MemoryManager final : public HiCR::MemoryManager
{
  public:

  /**
   * Constructor for the memory manager class for the OpenCL backend.
   * 
   * \param[in] deviceQueueMap map of device ids and command queues
   */
  MemoryManager(const std::unordered_map<opencl::Device::deviceIdentifier_t, std::shared_ptr<cl::CommandQueue>> &deviceQueueMap)
    : HiCR::MemoryManager(),
      _deviceQueueMap(deviceQueueMap)
  {}

  /**
   * Default destructor
  */
  ~MemoryManager() = default;

  private:

  __INLINE__ std::shared_ptr<HiCR::LocalMemorySlot> allocateLocalMemorySlotImpl(std::shared_ptr<HiCR::MemorySpace> memorySpace, const size_t size) override
  {
    // Getting up-casted pointer for the opencl instance, first try with the device memory space
    auto openclMemorySpace = dynamic_pointer_cast<opencl::MemorySpace>(memorySpace);
    auto hwlocMemorySpace  = dynamic_pointer_cast<hwloc::MemorySpace>(memorySpace);

    // Checking whether the memory space passed belongs to the device or to the host
    if (hwlocMemorySpace != nullptr) { return allocateLocalDeviceMemorySlot(memorySpace, size); }
    if (openclMemorySpace != nullptr) { return allocateLocalHostMemorySlot(memorySpace, size); }

    HICR_THROW_LOGIC("The passed memory space is not supported by this memory manager. Supported opencl and hwloc\n");
  }

  __INLINE__ std::shared_ptr<HiCR::LocalMemorySlot> allocateLocalDeviceMemorySlot(const std::shared_ptr<HiCR::MemorySpace> memorySpace, const size_t size)
  {
    cl_int err;
    auto   queue   = getQueue(memorySpace);
    auto   context = queue->getInfo<CL_QUEUE_CONTEXT>();

    // Create OpenCL buffer on the device ready for read and write operations
    auto deviceBuffer = std::make_shared<cl::Buffer>(context, CL_MEM_READ_WRITE, size, nullptr, &err);
    if (err != CL_SUCCESS) [[unlikely]] { HICR_THROW_RUNTIME("Can not allocate local memory slot on the device: %d", err); }

    // Map it to the host in order to get a pointer to the data. Data are mapped in memory for writing
    auto hostPtr = queue->enqueueMapBuffer(*deviceBuffer, CL_TRUE, CL_MAP_READ | CL_MAP_READ | CL_MAP_WRITE, 0, size, nullptr, nullptr, &err);
    if (err != CL_SUCCESS || hostPtr == nullptr) [[unlikely]] { HICR_THROW_RUNTIME("Can not retrieve pointer: %d", err); }

    auto memSlot = std::make_shared<opencl::LocalMemorySlot>(hostPtr, size, deviceBuffer, memorySpace);

    // create the new memory slot
    return memSlot;
  }

  __INLINE__ std::shared_ptr<HiCR::LocalMemorySlot> allocateLocalHostMemorySlot(const std::shared_ptr<HiCR::MemorySpace> memorySpace, const size_t size)
  {
    cl_int err;
    auto   queue   = getQueue(memorySpace);
    auto   context = queue->getInfo<CL_QUEUE_CONTEXT>();

    // Create OpenCL buffer
    auto hostBuffer = std::make_shared<cl::Buffer>(context, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, size, nullptr, &err);
    if (err != CL_SUCCESS) [[unlikely]] { HICR_THROW_RUNTIME("Can not allocate local memory slot on the host: %d", err); }

    // Map it to the host in order to get a pointer to the data. Data are mapped in memory for writing
    auto hostPtr = queue->enqueueMapBuffer(*hostBuffer, CL_TRUE, CL_MAP_READ | CL_MAP_WRITE, 0, size, nullptr, nullptr, &err);
    if (err != CL_SUCCESS || hostPtr == nullptr) [[unlikely]] { HICR_THROW_RUNTIME("Can not retrieve pointer: %d", err); }

    auto memSlot = std::make_shared<opencl::LocalMemorySlot>(hostPtr, size, hostBuffer, memorySpace);

    // create the new memory slot
    return memSlot;
  }

  /**
   * \note only pointers allocated on the host are supported
  */
  __INLINE__ std::shared_ptr<HiCR::LocalMemorySlot> registerLocalMemorySlotImpl(std::shared_ptr<HiCR::MemorySpace> memorySpace, void *const ptr, const size_t size) override
  {
    // Getting up-casted pointer for the opencl instance, first try with the device memory space
    auto openclMemorySpace = dynamic_pointer_cast<const opencl::MemorySpace>(memorySpace);

    // Checking whether the memory space passed belongs to the device or to the host
    if (openclMemorySpace != nullptr) [[unlikely]] { HICR_THROW_RUNTIME("Can not register local memory slot on the provided memory space: %s", memorySpace->getType().c_str()); }

    cl_int err;
    auto   queue   = getQueue(memorySpace);
    auto   context = queue->getInfo<CL_QUEUE_CONTEXT>();

    // Create OpenCL buffer
    auto buffer = std::make_shared<cl::Buffer>(context, CL_MEM_USE_HOST_PTR, size, ptr, &err);
    if (err != CL_SUCCESS) [[unlikely]] { HICR_THROW_RUNTIME("Can not register local memory slot on the host: %d", err); }

    // Map it to the host in order to get a pointer to the data. Data are mapped in memory for writing
    auto hostPtr = queue->enqueueMapBuffer(*buffer, CL_TRUE, CL_MAP_READ | CL_MAP_WRITE, 0, size, nullptr, nullptr, &err);
    if (err != CL_SUCCESS || hostPtr == nullptr) [[unlikely]] { HICR_THROW_RUNTIME("Can not retrieve pointer: %d", err); }

    // create the new memory slot
    return std::make_shared<opencl::LocalMemorySlot>(hostPtr, size, buffer, memorySpace);
  }

  __INLINE__ void memsetImpl(const std::shared_ptr<HiCR::LocalMemorySlot> memorySlot, int value, size_t size) override
  {
    auto m = dynamic_pointer_cast<opencl::LocalMemorySlot>(memorySlot);
    if (m == nullptr) [[unlikely]] { HICR_THROW_RUNTIME("Unsupported local memory slot: %s", memorySlot->getMemorySpace()->getType().c_str()); }

    cl_int clValue = value;

    auto completionEvent = cl::Event();
    auto queue           = getQueue(m->getMemorySpace());
    auto err             = queue->enqueueFillBuffer(*(m->getBuffer()), clValue, 0, m->getSize(), nullptr, &completionEvent);
    if (err != CL_SUCCESS) [[unlikely]] { HICR_THROW_RUNTIME("Can not perform memset: %d", err); }
    completionEvent.wait();
  }

  __INLINE__ void freeLocalMemorySlotImpl(std::shared_ptr<HiCR::LocalMemorySlot> memorySlot) override
  {
    auto m = dynamic_pointer_cast<opencl::LocalMemorySlot>(memorySlot);
    if (m == nullptr) [[unlikely]] { HICR_THROW_RUNTIME("Unsupported local memory slot: %s", memorySlot->getMemorySpace()->getType().c_str()); }

    auto queue  = getQueue(m->getMemorySpace());
    auto buffer = m->getBuffer();

    auto err = queue->enqueueUnmapMemObject(*buffer, m->getPointer());
    if (err != CL_SUCCESS) [[unlikely]] { HICR_THROW_RUNTIME("Can not unmap host pointer: %d", err); }

    m->getBuffer().reset();
  }

  /**
   * Backend-internal implementation of the deregisterMemorySlot function
   *
   * \param[in] memorySlot memory slot to deregister.
   */
  __INLINE__ void deregisterLocalMemorySlotImpl(std::shared_ptr<HiCR::LocalMemorySlot> memorySlot) override {}

  private:

  /**
   * Enumeration to indicate a type of device involved in data communication operations
   */
  enum memSpaceType_t
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
     * Device -- Involves an OpenCL device memory (DRAM) in the operation
     */
    device
  };

  /**
   * Map of queues per device
  */
  const std::unordered_map<opencl::Device::deviceIdentifier_t, std::shared_ptr<cl::CommandQueue>> _deviceQueueMap;

  /**
   * Get the queue associated to a memory space
   * 
   * \param[in] memorySpace
   * 
   * \return a OpenCL command queue
  */
  std::shared_ptr<cl::CommandQueue> getQueue(std::shared_ptr<HiCR::MemorySpace> memorySpace)
  {
    auto openclMemorySpace = dynamic_pointer_cast<opencl::MemorySpace>(memorySpace);
    auto hwlocMemorySpace  = dynamic_pointer_cast<hwloc::MemorySpace>(memorySpace);
    if (hwlocMemorySpace != nullptr) { return _deviceQueueMap.begin()->second; }
    if (openclMemorySpace != nullptr)
    {
      auto device   = openclMemorySpace->getDevice().lock();
      auto deviceId = device->getId();
      return _deviceQueueMap.at(deviceId);
    }
    HICR_THROW_LOGIC("The passed memory space is not supported by this memory manager. Supported opencl and hwloc\n");
  }
};

} // namespace HiCR::backend::opencl