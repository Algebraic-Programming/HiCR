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
#include <backends/ascend/L0/device.hpp>
#include <backends/ascend/L0/memorySpace.hpp>
#include <backends/ascend/L0/localMemorySlot.hpp>
#include <backends/sharedMemory/L0/memorySpace.hpp>
#include <hicr/L1/memoryManager.hpp>

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
   * Constructor for the ascend memory manager class for the ascend backend.
   */
  MemoryManager() : HiCR::L1::MemoryManager() {}
  ~MemoryManager() = default;

  private:

  __USED__ inline std::shared_ptr<HiCR::L0::LocalMemorySlot> allocateLocalMemorySlotImpl(std::shared_ptr<HiCR::L0::MemorySpace> memorySpace, const size_t size) override
  {
    // Getting up-casted pointer for the MPI instance
    auto ascendMemSpace = dynamic_pointer_cast<const ascend::L0::MemorySpace>(memorySpace);

    // Checking whether the execution unit passed is compatible with this backend
    if (ascendMemSpace != NULL) return allocateLocalDeviceMemorySlot(memorySpace, size);

    // Getting up-casted pointer for the MPI instance
    auto hostMemSpace = dynamic_pointer_cast<sharedMemory::L0::MemorySpace>(memorySpace);

    // Checking whether the execution unit passed is compatible with this backend
    if (hostMemSpace != NULL) return allocateLocalHostMemorySlot(memorySpace, size);

    HICR_THROW_LOGIC("The passed memory space is not supported by this memory manager\n");
  }

  __USED__ inline std::shared_ptr<HiCR::L0::LocalMemorySlot> allocateLocalDeviceMemorySlot(const std::shared_ptr<HiCR::L0::MemorySpace> memorySpace, const size_t size)
  {
    void *ptr = NULL;
    aclDataBuffer *dataBuffer;

    // do a malloc on the ascend and create the databuffer
    auto ascendMemSpace = dynamic_pointer_cast<ascend::L0::MemorySpace>(memorySpace);
    ptr = deviceAlloc(ascendMemSpace, size);
    dataBuffer = aclCreateDataBuffer(ptr, size);
    if (dataBuffer == NULL) HICR_THROW_RUNTIME("Can not create data buffer in device");

    // create the new memory slot
    return std::make_shared<L0::LocalMemorySlot>(ptr, size, dataBuffer, memorySpace);
  }

  __USED__ inline std::shared_ptr<HiCR::L0::LocalMemorySlot> allocateLocalHostMemorySlot(const std::shared_ptr<HiCR::L0::MemorySpace> memorySpace, const size_t size)
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
  __USED__ inline void *deviceAlloc(std::shared_ptr<ascend::L0::MemorySpace> memorySpace, const size_t size)
  {
    // Getting device associated with this memory space
    auto device = memorySpace->getDevice().lock();

    // select the device context on which we should allocate the memory
    device->select();

    // Storage for the allocation pointer
    void *ptr = nullptr;

    // Do the allocation on device memory
    aclError err = aclrtMalloc(&ptr, size, ACL_MEM_MALLOC_HUGE_FIRST);
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not allocate memory on ascend device %d. Error %d", device->getId(), err);

    // Returning allocated pointer
    return ptr;
  }

  /**
   * Allocate memory on the host memory through Ascend-dedicated functions.
   *
   * \param[in] memorySpace device id where memory is allocated
   * \param[in] size allocation size
   */
  __USED__ inline void *hostAlloc(std::shared_ptr<HiCR::L0::MemorySpace> memorySpace, const size_t size)
  {
    // Storage for the allocation pointer
    void *ptr = nullptr;

    // Do the allocation on device memory
    aclError err = aclrtMallocHost(&ptr, size);
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not allocate memory on host through ascend-dedicated function. Error %d", err);

    // Returning allocated pointer
    return ptr;
  }

  __USED__ inline std::shared_ptr<HiCR::L0::LocalMemorySlot> registerLocalMemorySlotImpl(std::shared_ptr<HiCR::L0::MemorySpace> memorySpace, void *const ptr, const size_t size) override
  {
    HICR_THROW_RUNTIME("Not yet implemented for this backend");
  }

  __USED__ inline void freeLocalMemorySlotImpl(std::shared_ptr<HiCR::L0::LocalMemorySlot> memorySlot) override
  {
    // Getting up-casted pointer for the execution unit
    auto m = dynamic_pointer_cast<ascend::L0::LocalMemorySlot>(memorySlot);

    // Checking whether the execution unit passed is compatible with this backend
    if (m == NULL) freeLocalHostMemorySlot(memorySlot);
    if (m != NULL) freeLocalDeviceMemorySlot(m);
  }

  __USED__ inline void freeLocalDeviceMemorySlot(std::shared_ptr<L0::LocalMemorySlot> memorySlot)
  {
    // Getting memory slot info
    const auto memorySlotPointer = memorySlot->getPointer();
    const auto memorySlotMemorySpace = dynamic_pointer_cast<HiCR::backend::ascend::L0::MemorySpace>(memorySlot->getMemorySpace());
    const auto memorySlotDevice = memorySlotMemorySpace->getDevice().lock();
    const auto memorySlotDeviceId = memorySlotDevice->getId();

    aclError err = aclrtFree(memorySlotPointer);
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Error while freeing device %d memory. Error %d", memorySlotDeviceId, err);

    err = aclDestroyDataBuffer(memorySlot->getDataBuffer());
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not destroy data buffer. Error %d", err);
  }

  __USED__ inline void freeLocalHostMemorySlot(std::shared_ptr<HiCR::L0::LocalMemorySlot> memorySlot)
  {
    aclError err = aclrtFreeHost(memorySlot->getPointer());
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Error while freeing host memory. Error %d", err);
  }

  /**
   * Backend-internal implementation of the deregisterMemorySlot function
   *
   * \param[in] memorySlot memory slot to deregister.
   */
  __USED__ inline void deregisterLocalMemorySlotImpl(std::shared_ptr<HiCR::L0::LocalMemorySlot> memorySlot) override
  {
    HICR_THROW_RUNTIME("Not yet implemented for this backend");
  }
};

} // namespace L1

} // namespace ascend

} // namespace backend

} // namespace HiCR
