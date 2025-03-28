/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file memoryManager.hpp
 * @brief This file implements the memory manager class for the Ascend backend
 * @author L. Terracciano & S. M. Martin
 * @date 11/9/2023
 */

#pragma once

#include <acl/acl.h>
#include <hicr/backends/ascend/L0/device.hpp>
#include <hicr/backends/ascend/L0/memorySpace.hpp>
#include <hicr/backends/ascend/L0/localMemorySlot.hpp>
#include <hicr/backends/hwloc/L0/memorySpace.hpp>
#include <hicr/core/L1/memoryManager.hpp>

namespace HiCR
{

namespace backend
{

namespace ascend
{

namespace L1
{

/**
 * Implementation of the Memory Manager for the Ascend backend.
 *
 * It stores the memory spaces detected by the Ascend computing language
 */
class MemoryManager final : public HiCR::L1::MemoryManager
{
  public:

  /**
   * Constructor for the ascend memory manager class for the Ascend backend.
   */
  MemoryManager()
    : HiCR::L1::MemoryManager()
  {}
  ~MemoryManager() = default;

  private:

  __INLINE__ std::shared_ptr<HiCR::L0::LocalMemorySlot> allocateLocalMemorySlotImpl(std::shared_ptr<HiCR::L0::MemorySpace> memorySpace, const size_t size) override
  {
    // Getting up-casted pointer for the ascend instance, first try with the device memory space
    auto ascendMemSpace = dynamic_pointer_cast<const ascend::L0::MemorySpace>(memorySpace);

    // Checking whether the memory space passed belongs to the device
    if (ascendMemSpace != NULL) return allocateLocalDeviceMemorySlot(memorySpace, size);

    // Retry with the host memory space
    auto hostMemSpace = dynamic_pointer_cast<hwloc::L0::MemorySpace>(memorySpace);

    // Checking whether the memory space passed belongs to the host
    if (hostMemSpace != NULL) return allocateLocalHostMemorySlot(memorySpace, size);

    HICR_THROW_LOGIC("The passed memory space is not supported by this memory manager. Supported ascend and hwloc\n");
  }

  __INLINE__ std::shared_ptr<HiCR::L0::LocalMemorySlot> allocateLocalDeviceMemorySlot(const std::shared_ptr<HiCR::L0::MemorySpace> memorySpace, const size_t size)
  {
    void          *ptr = NULL;
    aclDataBuffer *dataBuffer;

    // do a malloc on the ascend and create the databuffer
    auto ascendMemSpace = dynamic_pointer_cast<ascend::L0::MemorySpace>(memorySpace);
    ptr                 = deviceAlloc(ascendMemSpace, size);
    dataBuffer          = aclCreateDataBuffer(ptr, size);
    if (dataBuffer == NULL) HICR_THROW_RUNTIME("Can not create data buffer in device");

    // create the new memory slot
    return std::make_shared<L0::LocalMemorySlot>(ptr, size, dataBuffer, memorySpace);
  }

  __INLINE__ std::shared_ptr<HiCR::L0::LocalMemorySlot> allocateLocalHostMemorySlot(const std::shared_ptr<HiCR::L0::MemorySpace> memorySpace, const size_t size)
  {
    void *ptr = NULL;

    // do a malloc on the ascend and create the databuffer
    ptr = hostAlloc(memorySpace, size);

    // create the new memory slot
    return std::make_shared<HiCR::L0::LocalMemorySlot>(ptr, size, memorySpace);
  }

  /**
   * Allocate memory on the Ascend memory through Ascend-dedicated functions.
   *
   * \param[in] memorySpace device id where memory is allocated
   * \param[in] size allocation size
   */
  __INLINE__ void *deviceAlloc(std::shared_ptr<ascend::L0::MemorySpace> memorySpace, const size_t size)
  {
    // Getting device associated with this memory space
    auto device = memorySpace->getDevice().lock();

    // select the device context on which we should allocate the memory
    device->select();

    // Storage for the allocation pointer
    void *ptr = nullptr;

    // Do the allocation on device memory
    aclError err = aclrtMalloc(&ptr, size, ACL_MEM_MALLOC_HUGE_FIRST);
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not allocate memory on Ascend device %d. Error %d", device->getId(), err);

    // Returning allocated pointer
    return ptr;
  }

  /**
   * Allocate memory on the host memory through Ascend-dedicated functions.
   *
   * \param[in] memorySpace device id where memory is allocated
   * \param[in] size allocation size
   */
  __INLINE__ void *hostAlloc(std::shared_ptr<HiCR::L0::MemorySpace> memorySpace, const size_t size)
  {
    // Storage for the allocation pointer
    void *ptr = nullptr;

    // Do the allocation on device memory
    aclError err = aclrtMallocHost(&ptr, size);
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not allocate memory on host through ascend-dedicated function. Error %d", err);

    // Returning allocated pointer
    return ptr;
  }

  __INLINE__ std::shared_ptr<HiCR::L0::LocalMemorySlot> registerLocalMemorySlotImpl(std::shared_ptr<HiCR::L0::MemorySpace> memorySpace, void *const ptr, const size_t size) override
  {
    return std::make_shared<HiCR::L0::LocalMemorySlot>(ptr, size, memorySpace);
  }

  __INLINE__ void memsetImpl(const std::shared_ptr<HiCR::L0::LocalMemorySlot> memorySlot, int value, size_t size) override
  {
    // Filling the memory slot with the provided value
    // Ascend aclrtMemset() automatically understands if the memory resides on the device or the host, so we can use it directly
    aclError err = aclrtMemset(memorySlot->getPointer(), memorySlot->getSize(), value, size);
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Error while performing memset. Error %d", err);
  }

  __INLINE__ void freeLocalMemorySlotImpl(std::shared_ptr<HiCR::L0::LocalMemorySlot> memorySlot) override
  {
    // Getting up-casted pointer for the memory slot
    auto m = dynamic_pointer_cast<ascend::L0::LocalMemorySlot>(memorySlot);

    // Checking whether the memory slot passed is compatible with this backend
    if (m == NULL) freeLocalHostMemorySlot(memorySlot);
    if (m != NULL) freeLocalDeviceMemorySlot(m);
  }

  __INLINE__ void freeLocalDeviceMemorySlot(std::shared_ptr<L0::LocalMemorySlot> memorySlot)
  {
    // Getting memory slot info
    const auto memorySlotPointer     = memorySlot->getPointer();
    const auto memorySlotMemorySpace = dynamic_pointer_cast<HiCR::backend::ascend::L0::MemorySpace>(memorySlot->getMemorySpace());
    const auto memorySlotDevice      = memorySlotMemorySpace->getDevice().lock();
    const auto memorySlotDeviceId    = memorySlotDevice->getId();

    aclError err = aclrtFree(memorySlotPointer);
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Error while freeing device %d memory. Error %d", memorySlotDeviceId, err);

    err = aclDestroyDataBuffer(memorySlot->getDataBuffer());
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not destroy data buffer. Error %d", err);
  }

  __INLINE__ void freeLocalHostMemorySlot(std::shared_ptr<HiCR::L0::LocalMemorySlot> memorySlot)
  {
    aclError err = aclrtFreeHost(memorySlot->getPointer());
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Error while freeing host memory. Error %d", err);
  }

  /**
   * Backend-internal implementation of the deregisterMemorySlot function
   *
   * \param[in] memorySlot memory slot to deregister.
   */
  __INLINE__ void deregisterLocalMemorySlotImpl(std::shared_ptr<HiCR::L0::LocalMemorySlot> memorySlot) override {}
};

} // namespace L1

} // namespace ascend

} // namespace backend

} // namespace HiCR
