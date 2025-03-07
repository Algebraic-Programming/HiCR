/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file communicationManager.hpp
 * @brief This file implements the communication manager class for the OpenCL backend
 * @author L. Terracciano
 * @date 06/03/2025
 */

#pragma once

#include <CL/opencl.hpp>
#include <hicr/backends/opencl/L0/memorySpace.hpp>
#include <hicr/backends/opencl/L0/device.hpp>
#include <hicr/backends/opencl/L0/localMemorySlot.hpp>
#include <hicr/backends/opencl/L1/memoryManager.hpp>
#include <hicr/core/L1/communicationManager.hpp>
#include <hicr/core/L0/globalMemorySlot.hpp>

namespace HiCR
{

namespace backend
{

namespace opencl
{

namespace L1
{

/**
 * Implementation of the Communication Manager for the OpenCL backend.
 */
class CommunicationManager final : public HiCR::L1::CommunicationManager
{
  public:

  /**
   * Constructor for the opencl communication manager class for the opencl backend.
   * 
   * \param[in] deviceQueueMap map of device ids and command queues
   */
  CommunicationManager(const std::unordered_map<opencl::L0::Device::deviceIdentifier_t, std::shared_ptr<cl::CommandQueue>> &deviceQueueMap)
    : HiCR::L1::CommunicationManager(),
      _deviceQueueMap(deviceQueueMap)
  {}

  ~CommunicationManager() = default;

  /**
   * Backend-internal asyncrhonous implementation of the memcpy operation. It passes an OpenCL stream as context for later asynchrounous check for completion
   *
   * For more information, see: memcpyImpl
   *
   * \param[in] destination destination memory slot
   * \param[in] dst_offset destination offset
   * \param[in] source source memory slot
   * \param[in] src_offset source offset
   * \param[in] size the number of bytes to copy
   * \param[in] queue opencl command queue that contains the state of the operation for later check
   */
  __INLINE__ void memcpyAsync(const std::shared_ptr<HiCR::L0::LocalMemorySlot> &destination,
                              const size_t                                      dst_offset,
                              const std::shared_ptr<HiCR::L0::LocalMemorySlot> &source,
                              const size_t                                      src_offset,
                              const size_t                                      size,
                              const cl::CommandQueue                           *queue)
  {
    memcpyInternal(destination, dst_offset, source, src_offset, size, queue);
  }

  private:

  __INLINE__ void exchangeGlobalMemorySlotsImpl(const HiCR::L0::GlobalMemorySlot::tag_t tag, const std::vector<globalKeyMemorySlotPair_t> &memorySlots) override
  {
    HICR_THROW_RUNTIME("Not yet implemented for this backend");
  }

  __INLINE__ void queryMemorySlotUpdatesImpl(std::shared_ptr<HiCR::L0::LocalMemorySlot> memorySlot) override { HICR_THROW_RUNTIME("Not yet implemented for this backend"); }

  __INLINE__ std::shared_ptr<HiCR::L0::GlobalMemorySlot> getGlobalMemorySlotImpl(const HiCR::L0::GlobalMemorySlot::tag_t       tag,
                                                                                 const HiCR::L0::GlobalMemorySlot::globalKey_t globalKey) override
  {
    return nullptr;
  }

  /**
   * Deletes a global memory slot from the backend.
   * Attempting to access the global memory slot after this operation will result in undefined behavior.
   *
   * \param[in] memorySlot Memory slot to destroy.
   */
  __INLINE__ void destroyGlobalMemorySlotImpl(const std::shared_ptr<HiCR::L0::GlobalMemorySlot> memorySlot) override { HICR_THROW_RUNTIME("Not yet implemented for this backend"); }

  /**
   * This memcpy implementation does support asynchronous inter-device communication, meaning the fence should be called when date are
   * moved among different opencl devices.
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
  __INLINE__ void memcpyImpl(const std::shared_ptr<HiCR::L0::LocalMemorySlot> &destination,
                             const size_t                                      dst_offset,
                             const std::shared_ptr<HiCR::L0::LocalMemorySlot> &source,
                             const size_t                                      src_offset,
                             const size_t                                      size) override
  {
    memcpyInternal(destination, dst_offset, source, src_offset, size, nullptr);
  }

  __INLINE__ void memcpyInternal(const std::shared_ptr<HiCR::L0::LocalMemorySlot> &destination,
                                 const size_t                                      dst_offset,
                                 const std::shared_ptr<HiCR::L0::LocalMemorySlot> &source,
                                 const size_t                                      src_offset,
                                 const size_t                                      size,
                                 const cl::CommandQueue                           *queue)
  {
    cl_int err = CL_SUCCESS;
    // Using up-casting to determine device types
    auto src = dynamic_pointer_cast<opencl::L0::LocalMemorySlot>(source);
    auto dst = dynamic_pointer_cast<opencl::L0::LocalMemorySlot>(destination);

    // Checking whether the memory slot unit passed is compatible with this backend
    if (src == nullptr) HICR_THROW_LOGIC("The passed source memory slot is not supported by this backend\n");
    if (dst == nullptr) HICR_THROW_LOGIC("The passed destination memory slot is not supported by this backend\n");

    // Async version
    if (queue != nullptr)
    {
      // Unmap src buffer
      unmap(queue, src);

      // Unmap dst buffer
      unmap(queue, dst);

      err = queue->enqueueCopyBuffer(*(src->getBuffer()), *(dst->getBuffer()), src_offset, dst_offset, size, nullptr, nullptr);
      if (err != CL_SUCCESS) { HICR_THROW_RUNTIME("Can not perform memcpy. Err: %d", err); }
    }

    // Sync version
    if (queue == nullptr)
    {
      queue = getQueue(dst->getMemorySpace()).get();

      // Unmap src buffer
      unmap(queue, src);

      // Unmap dst buffer
      unmap(queue, dst);

      cl::Event completionEvent;
      err = queue->enqueueCopyBuffer(*(src->getBuffer()), *(dst->getBuffer()), src_offset, dst_offset, size, nullptr, &completionEvent);
      if (err != CL_SUCCESS) { HICR_THROW_RUNTIME("Can not perform memcpy. Err: %d", err); }
      completionEvent.wait();
    }

    // Map the src buffer
    map(queue, src);

    // Map the dst buffer
    map(queue, dst);

    // Increasing message received/sent counters for both memory slots
    increaseMessageRecvCounter(*destination);
    increaseMessageSentCounter(*source);
  }

  __INLINE__ void fenceImpl(const HiCR::L0::GlobalMemorySlot::tag_t tag) override {}

  __INLINE__ bool acquireGlobalLockImpl(std::shared_ptr<HiCR::L0::GlobalMemorySlot> memorySlot) override { HICR_THROW_RUNTIME("Not yet implemented for this backend"); }

  __INLINE__ void releaseGlobalLockImpl(std::shared_ptr<HiCR::L0::GlobalMemorySlot> memorySlot) override { HICR_THROW_RUNTIME("Not yet implemented for this backend"); }

  private:

  /**
   * Map of queues per device
  */
  const std::unordered_map<opencl::L0::Device::deviceIdentifier_t, std::shared_ptr<cl::CommandQueue>> _deviceQueueMap;

  /**
   * Get the queue associated to a memory space
   * 
   * \param[in] memorySpace
   * 
   * \return a OpenCL command queue
  */
  std::shared_ptr<cl::CommandQueue> getQueue(std::shared_ptr<HiCR::L0::MemorySpace> memorySpace)
  {
    auto openclMemorySpace = dynamic_pointer_cast<opencl::L0::MemorySpace>(memorySpace);
    auto hwlocMemorySpace  = dynamic_pointer_cast<hwloc::L0::MemorySpace>(memorySpace);
    if (hwlocMemorySpace != nullptr) { return _deviceQueueMap.begin()->second; }
    if (openclMemorySpace != nullptr)
    {
      auto device   = openclMemorySpace->getDevice().lock();
      auto deviceId = device->getId();
      return _deviceQueueMap.at(deviceId);
    }
    HICR_THROW_LOGIC("The passed memory space is not supported by this memory manager. Supported opencl and hwloc\n");
  }

  /**
   * Map the memory slot
   * 
   * \param[in] queue OpenCL command queue
   * \param[in] memSlot the memory slot to unmap
  */
  __INLINE__ void map(const cl::CommandQueue *queue, std::shared_ptr<HiCR::backend::opencl::L0::LocalMemorySlot> &memSlot)
  {
    cl_int err;
    memSlot->getPointer() = queue->enqueueMapBuffer(*(memSlot->getBuffer()), CL_TRUE, CL_MAP_READ | CL_MAP_WRITE, 0, memSlot->getSize(), nullptr, nullptr, &err);
    if (err != CL_SUCCESS) { HICR_THROW_RUNTIME("Can not map the buffer. Error: %d", err); }
  }

  /**
   * Unmap the memory slot
   * 
   * \param[in] queue OpenCL command queue
   * \param[in] memSlot the memory slot to unmap
  */
  __INLINE__ void unmap(const cl::CommandQueue *queue, std::shared_ptr<HiCR::backend::opencl::L0::LocalMemorySlot> &memSlot)
  {
    auto err = queue->enqueueUnmapMemObject(*(memSlot->getBuffer()), memSlot->getPointer(), nullptr, nullptr);
    if (err != CL_SUCCESS) { HICR_THROW_RUNTIME("Can not unmap the buffer. Error: %d", err); }
  }
};

} // namespace L1

} // namespace opencl

} // namespace backend

} // namespace HiCR
