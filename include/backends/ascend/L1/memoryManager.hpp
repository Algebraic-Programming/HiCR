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
#include <backends/sequential/L0/memorySpace.hpp>
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

  /**
   * Backend-internal implementation of the queryLocalMemorySlot function
   *
   * \param[in] memorySpaceId memory space to allocate memory in
   * \param[in] size size of the memory slot to create
   * \return the internal pointer associated to the local memory slot
   */
  __USED__ inline HiCR::L0::LocalMemorySlot *allocateLocalMemorySlotImpl(HiCR::L0::MemorySpace *memorySpace, const size_t size) override
  {
    // Getting up-casted pointer for the MPI instance
    auto ascendMemSpace = dynamic_cast<const ascend::L0::MemorySpace *>(memorySpace);

    // Checking whether the execution unit passed is compatible with this backend
    if (ascendMemSpace != NULL) return allocateLocalDeviceMemorySlot(ascendMemSpace, size);

    // Getting up-casted pointer for the MPI instance
    auto hostMemSpace = dynamic_cast<const sequential::L0::MemorySpace *>(memorySpace);

    // Checking whether the execution unit passed is compatible with this backend
    if (hostMemSpace != NULL) return allocateLocalHostMemorySlot(hostMemSpace, size);

    HICR_THROW_LOGIC("The passed memory space is not supported by this memory manager\n");
  }

  __USED__ inline HiCR::L0::LocalMemorySlot *allocateLocalDeviceMemorySlot(const HiCR::backend::ascend::L0::MemorySpace *memorySpace, const size_t size)
  {
    void *ptr = NULL;
    aclDataBuffer *dataBuffer;

    // do a malloc on the ascend and create the databuffer
    ptr = deviceAlloc(memorySpace, size);
    dataBuffer = aclCreateDataBuffer(ptr, size);
    if (dataBuffer == NULL) HICR_THROW_RUNTIME("Can not create data buffer in device");

    // create the new memory slot
    return new L0::LocalMemorySlot(ptr, size, dataBuffer, (HiCR::L0::MemorySpace *)memorySpace);
  }

  __USED__ inline HiCR::L0::LocalMemorySlot *allocateLocalHostMemorySlot(const HiCR::backend::sequential::L0::MemorySpace *memorySpace, const size_t size)
  {
    void *ptr = NULL;

    // do a malloc on the ascend and create the databuffer
    ptr = hostAlloc(memorySpace, size);

    // create the new memory slot
    return new HiCR::L0::LocalMemorySlot(ptr, size, (HiCR::L0::MemorySpace *)memorySpace);
  }

  /**
   * Allocate memory on the Ascend memory through Ascend-dedicated functions.
   *
   * \param memorySpace device id where memory is allocated
   * \param size allocation size
   */
  __USED__ inline void *deviceAlloc(const ascend::L0::MemorySpace *memorySpace, const size_t size)
  {
    // Getting device associated with this memory space
    auto device = memorySpace->getDevice();

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
   * \param memorySpace device id where memory is allocated
   * \param size allocation size
   */
  __USED__ inline void *hostAlloc(const sequential::L0::MemorySpace *memorySpace, const size_t size)
  {
    // Storage for the allocation pointer
    void *ptr = nullptr;

    // Do the allocation on device memory
    aclError err = aclrtMallocHost(&ptr, size);
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not allocate memory on host through ascend-dedicated function. Error %d", err);

    // Returning allocated pointer
    return ptr;
  }
  /**
   * Backend-internal implementation of the registerLocalMemorySlot function. Not implemented.
   *
   * \param ptr pointer to the start of the memory slot
   * \param size size of the memory slot to create
   * \return a newly created memory slot
   */
  __USED__ inline HiCR::L0::LocalMemorySlot *registerLocalMemorySlotImpl(HiCR::L0::MemorySpace *memorySpace, void *const ptr, const size_t size) override
  {
    HICR_THROW_RUNTIME("Not yet implemented for this backend");
  }

  /**
   * Backend-internal implementation of the freeLocalMemorySlot function
   *
   * \param memorySlot local memory slot to free up. It becomes unusable after freeing.
   */
  __USED__ inline void freeLocalMemorySlotImpl(HiCR::L0::LocalMemorySlot *memorySlot) override
  {
    // Getting up-casted pointer for the execution unit
    auto m = dynamic_cast<ascend::L0::LocalMemorySlot *>(memorySlot);

    // Checking whether the execution unit passed is compatible with this backend
    if (m != NULL)
    {
      freeLocalDeviceMemorySlot(m);
    }
    else
    {
      freeLocalHostMemorySlot(memorySlot);
    }
  }

  __USED__ inline void freeLocalDeviceMemorySlot(L0::LocalMemorySlot *memorySlot)
  {
    // Getting memory slot info
    const auto memorySlotPointer = memorySlot->getPointer();
    const auto memorySlotMemorySpace = (ascend::L0::MemorySpace *)memorySlot->getMemorySpace();
    const auto memorySlotDevice = memorySlotMemorySpace->getDevice();
    const auto memorySlotDeviceId = memorySlotDevice->getId();

    aclError err = aclrtFree(memorySlotPointer);
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Error while freeing device %d memory. Error %d", memorySlotDeviceId, err);

    err = aclDestroyDataBuffer(memorySlot->getDataBuffer());
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not destroy data buffer. Error %d", err);
  }

  __USED__ inline void freeLocalHostMemorySlot(HiCR::L0::LocalMemorySlot *memorySlot)
  {
    aclError err = aclrtFreeHost(memorySlot->getPointer());
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Error while freeing host memory. Error %d", err);
  }

  /**
   * Backend-internal implementation of the deregisterMemorySlot function
   *
   * \param memorySlot memory slot to deregister.
   */
  __USED__ inline void deregisterLocalMemorySlotImpl(HiCR::L0::LocalMemorySlot *memorySlot) override
  {
    HICR_THROW_RUNTIME("Not yet implemented for this backend");
  }
};

} // namespace L1

} // namespace ascend

} // namespace backend

} // namespace HiCR
