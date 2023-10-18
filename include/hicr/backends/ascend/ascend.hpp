/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file ascend.hpp
 * @brief This is a minimal backend for Ascend execution support
 * @author S. M. Martin & L. Terracciano
 * @date 06/10/2023
 */

#pragma once

#include <acl/acl.h>
#include <chrono>
#include <hccl/hccl.h>
#include <hicr/backend.hpp>
#include <hicr/common/definitions.hpp>
#include <thread>

namespace HiCR
{

namespace backend
{

namespace ascend
{

/**
 * Implementation of the HiCR Ascend backend
 *
 */
class Ascend final : public Backend
{
  public:

  /**
   * Constructor for the ascend backend.
   */
  Ascend(const char *config_path = NULL) : Backend()
  {
    aclError err;
    err = aclInit(config_path);

    // TODO: discuss with Sergio how we should handle these errors in the constructor
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Failed to initialize Ascend Computing Language. Error %d", err);
  }

  ~Ascend()
  {
    for (uint32_t deviceId = 0; deviceId < deviceCount; ++deviceId)
    {
      (void)HcclCommDestroy(comms[deviceId]);
    }
    free(comms);

    for (const auto memAscendData : _memoryAscendMap)
    {
      if (memAscendData.second.deviceId == _deviceStatusMap.size() - 1)
      {
        (void)aclrtFreeHost((void *)memAscendData.second.pointer);
      }
      else
      {
        // TODO: better way is to group by device id
        (void)selectDevice(memAscendData.second.deviceId);
        (void)aclrtFree((void *)memAscendData.second.pointer);
      }
    }

    _memoryAscendMap.clear();

    // for (const auto dataBufferData : _memoryDataBufferMap)
    // {
    //   (void)aclDestroyDataBuffer(dataBufferData.second);
    // }

    // _memoryDataBufferMap.clear();

    for (const auto memSpaceData : _deviceStatusMap)
    {
      (void)aclrtDestroyContext(memSpaceData.second.context);
    }

    _deviceStatusMap.clear();
    printf("Ascend backend destroyed\n");

    (void)aclFinalize();
  }

  private:

  typedef uint64_t deviceIdentifier_t;

  /**
   * Keeps track of how many devices are connected to the host
   */
  uint32_t deviceCount;

  HcclComm *comms;

  struct globalAscendSlot_t
  {
    /**
     * remember the device of which this slot is local
     */
    deviceIdentifier_t deviceId;
    /**
     * Pointer to the local memory address containing this slot
     */
    const void *pointer;

    /**
     * Size of the memory slot
     */
    size_t size;
  };

  struct ascendState_t
  {
    /**
     * remember the context associated to a device
     */
    aclrtContext context;

    /**
     * remember the device of which this slot is local
     */
    deviceIdentifier_t deviceId;

    /**
     * memory size of the device
     */
    size_t size;
  };

  /**
   * Keep track of the context for each deviceId. The last one is reserved for the host memory allocation
   */
  parallelHashMap_t<memorySpaceId_t, ascendState_t> _deviceStatusMap;

  /**
   * Keep track of the hccl connectors between two ascend cards.
   */
  parallelHashMap_t<memorySpaceId_t, parallelHashMap_t<memorySpaceId_t, HcclComm>> _deviceHcclConnectorsMap;

  /**
   *  Keep track of which devices contains a memory slot
   */
  std::map<const void *, globalAscendSlot_t> _memoryAscendMap;

  /**
   * Needed only for executing kernels. probably not necessary for memcpy
   */
  std::map<const void *, aclDataBuffer *> _memoryDataBufferMap;

  __USED__ inline void selectDevice(const memorySpaceId_t memorySpace)
  {
    aclError err;

    // select the device context on which we should allocate the memory
    err = aclrtSetCurrentContext(_deviceStatusMap[memorySpace].context);
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not set ascend device %d. Error %d", memorySpace, err);
  }

  /**
   * This function returns the available allocatable size in the current system RAM
   *
   * @param[in] memorySpace Always zero, represents the system's RAM
   * @return The allocatable size within the system
   */
  __USED__ inline size_t getMemorySpaceSizeImpl(const memorySpaceId_t memorySpace) const override
  {
    const auto deviceState = _deviceStatusMap.at(memorySpace);

    return deviceState.size;
  }

  /**
   * Ascend backend implementation that returns a single compute element.
   */
  __USED__ inline computeResourceList_t queryComputeResourcesImpl() override
  {
    HICR_THROW_LOGIC("This function has not been implemented yet");
  }

  /**
   * Ascend backend implementation that returns a single memory space representing the entire RAM device memory.
   */
  __USED__ inline memorySpaceList_t queryMemorySpacesImpl() override
  {
    const auto memorySpaceList = createMemorySpacesListAndSetupContexts();

    printf("prepare hccl\n");
    // Setup a single-process multiple card communication
    HcclResult err;
    int32_t devices[deviceCount];
    for (uint32_t deviceId = 0; deviceId < deviceCount; ++deviceId)
    {
      devices[deviceId] = deviceId;
    }

    comms = (HcclComm *)malloc(deviceCount * sizeof(HcclComm));
    err = HcclCommInitAll(deviceCount, devices, comms);
    if (err != HCCL_SUCCESS) HICR_THROW_RUNTIME("Failed to initialize HCCL. Error %d", err);
    printf("prepared hccl\n");
    return memorySpaceList;
  }

  /**
   * Ascend backend implementation that returns a single memory space representing the entire RAM device memory.
   */
  __USED__ inline memorySpaceList_t createMemorySpacesListAndSetupContexts()
  {
    // Clearing existing memory space map
    _deviceStatusMap.clear();

    // New memory space list to return
    memorySpaceList_t memorySpaceList;

    // Ask ACL for available devices
    aclError err;
    err = aclrtGetDeviceCount(&deviceCount);
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not retrieve ascend device count. Error %d", err);

    size_t size, totalMemory;
    aclrtContext deviceContext;

    // Add as many memory spaces as devices
    for (uint32_t deviceId = 0; deviceId < deviceCount; ++deviceId)
    {
      err = aclrtCreateContext(&deviceContext, deviceId);
      if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not create context in ascend device %d. Error %d", deviceId, err);

      err = aclrtSetCurrentContext(deviceContext);
      if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not create context in ascend device %d. Error %d", deviceId, err);

      err = aclrtGetMemInfo(ACL_DDR_MEM, &size, &totalMemory);
      if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not retrieve ascend device %d memory space. Error %d", deviceId, err);

      // TODO: do we care about total memory?
      err = aclrtGetMemInfo(ACL_HBM_MEM, &size, &totalMemory);
      if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not retrieve ascend device %d memory space. Error %d", deviceId, err);

      _deviceStatusMap[deviceId] = ascendState_t{.context = deviceContext, .deviceId = deviceId, .size = size};
      memorySpaceList.insert(deviceId);
    }

    // init host context
    // TODO: get host size
    const auto hostMemorySize = getTotalSystemMemory();
    _deviceStatusMap[deviceCount] = ascendState_t{.deviceId = deviceCount, .size = hostMemorySize};
    memorySpaceList.insert(deviceCount);

    return memorySpaceList;
  }

  /**
   * This function returns the system physical memory size, which is what matters for a sequential program
   *
   * This is adapted from https://stackoverflow.com/a/2513561
   */
  // TODO: borrowed from sequential backend. make it accessible also to ascend?
  __USED__ inline static size_t getTotalSystemMemory()
  {
    size_t pages = sysconf(_SC_PHYS_PAGES);
    size_t page_size = sysconf(_SC_PAGE_SIZE);
    return pages * page_size;
  }

  __USED__ inline std::unique_ptr<ProcessingUnit> createProcessingUnitImpl(computeResourceId_t resource) const override
  {
    HICR_THROW_LOGIC("This function has not been implemented yet");
  }

  /**
   * Backend-internal memcpy implementation.
   * Restrictions: Only memory copying between Devices in the same thread or between different threads in the same process is supported.
   * Memory copying between Devices in different processes is not supported.
   */
  __USED__ inline void memcpyImpl(MemorySlot *destination, const size_t dst_offset, MemorySlot *source, const size_t src_offset, const size_t size) override
  {
    if (!isMemorySlotValid(source)) HICR_THROW_RUNTIME("Invalid source memory slot(s) (%lu) provided. It either does not exist or is invalid", source);
    if (!isMemorySlotValid(destination)) HICR_THROW_RUNTIME("Invalid destination memory slot(s) (%lu) provided. It either does not exist or is invalid", destination);

    aclError err;
    // TODO: async?
    // Getting slot pointers
    const auto srcPtr = source->getPointer();
    const auto srcDeviceData = _memoryAscendMap.at(srcPtr);
    const auto srcDeviceId = srcDeviceData.deviceId;
    const auto dstPtr = destination->getPointer();
    const auto dstDeviceData = _memoryAscendMap.at(dstPtr);
    const auto dstDeviceId = dstDeviceData.deviceId;

    // Calculating actual offsets
    const auto actualSrcPtr = (void *)((uint8_t *)srcPtr + src_offset);
    const auto actualDstPtr = (void *)((uint8_t *)dstPtr + dst_offset);

    // TODO: check if there is enough space?

    // TODO: different default
    aclrtMemcpyKind memcpyKind = ACL_MEMCPY_HOST_TO_HOST;

    // TODO: Discriminate between host and device memory
    // src = host dst = host
    if (srcDeviceId == _deviceStatusMap.size() - 1 && dstDeviceId == _deviceStatusMap.size() - 1)
    {
      printf("copying from host to host\n");
      memcpyKind = ACL_MEMCPY_HOST_TO_HOST;
    }
    // src = host dst = device
    else if (srcDeviceId == _deviceStatusMap.size() - 1 && dstDeviceId < _deviceStatusMap.size() - 1)
    {
      printf("copying from host to device\n");
      selectDevice(dstDeviceId);
      memcpyKind = ACL_MEMCPY_HOST_TO_DEVICE;
    }
    // src = device dst = host
    else if (srcDeviceId < _deviceStatusMap.size() - 1 && dstDeviceId == _deviceStatusMap.size() - 1)
    {
      printf("copying from device to host\n");
      selectDevice(srcDeviceId);
      memcpyKind = ACL_MEMCPY_DEVICE_TO_HOST;
    }
    // src = deviceX dst = deviceX
    else if (srcDeviceId < _deviceStatusMap.size() - 1 && dstDeviceId == srcDeviceId)
    {
      // TODO: setup HCCL instead?
      printf("copying from device to same device\n");
      selectDevice(srcDeviceId);
      memcpyKind = ACL_MEMCPY_DEVICE_TO_DEVICE;
    }
    // src = deviceX dst = deviceY
    else if (srcDeviceId < _deviceStatusMap.size() - 1 && dstDeviceId < _deviceStatusMap.size() - 1 && dstDeviceId != srcDeviceId)
    {
      aclrtStream dstStream;
      aclError err;
      HcclResult hcclErr;
      // TODO: setup HCCL instead?
      printf("copying from device to device\n");
      selectDevice(srcDeviceId);
      uint32_t srcRankId, srcRankSize;
      hcclErr = HcclGetRankId(comms[srcDeviceId], &srcRankId);
      if (hcclErr != HCCL_SUCCESS) HICR_THROW_RUNTIME("Can not get source rank id in communication. Error %d", hcclErr);
      hcclErr = HcclGetRankSize(comms[srcDeviceId], &srcRankSize);
      if (hcclErr != HCCL_SUCCESS) HICR_THROW_RUNTIME("Can not get source rank size in communication. Error %d", hcclErr);
      printf("src dev id %ld rank id %d rank size %d\n", srcDeviceId, srcRankId, srcRankSize);

      selectDevice(dstDeviceId);
      uint32_t dstRankId, dstRankSize;
      hcclErr = HcclGetRankId(comms[dstDeviceId], &dstRankId);
      if (hcclErr != HCCL_SUCCESS) HICR_THROW_RUNTIME("Can not get dest rank id in communication. Error %d", hcclErr);
      hcclErr = HcclGetRankSize(comms[dstDeviceId], &dstRankSize);
      if (hcclErr != HCCL_SUCCESS) HICR_THROW_RUNTIME("Can not get dest rank size in communication. Error %d", hcclErr);

      printf("dst dev id %ld rank id %d rank size %d\n", dstDeviceId, dstRankId, dstRankSize);

      std::mutex _mutex;
      auto srcThread = std::thread(&Ascend::sendDataHccl, this, srcRankId, actualSrcPtr, size, dstRankId, std::ref(_mutex));
      // auto dstThread = std::thread(&Ascend::recvDataHccl, this, dstDeviceId, actualDstPtr, size, srcDeviceId);

      // srcThread.join();
      // dstThread.join();

      // selectDevice(srcDeviceId  );

      // err = aclrtCreateStream(&srcStream);
      // if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("failed to create src stream. Error %d", err);

      // printf("sending data \n");
      // hcclErr = HcclSend(actualSrcPtr, size, HCCL_DATA_TYPE_INT8, dstDeviceId, comms[0], srcStream);
      // if (hcclErr != HCCL_SUCCESS) HICR_THROW_RUNTIME("failed to send data via hccl. Error %d", hcclErr);
      // // printf("sync stream send\n");
      // // err = aclrtSynchronizeStream(srcStream);
      // // if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("failed to sync stream on send data via hccl. Error %d", err);
      // printf("data sent\n");

      // // err = aclrtDestroyStream(stream);
      // // if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("failed to destroy stream in send data hccl. Error %d", err);
      std::this_thread::sleep_for(std::chrono::seconds(2));
      _mutex.lock();

      printf("locekd recv\n");
      selectDevice(dstDeviceId);

      err = aclrtCreateStream(&dstStream);
      if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("failed to create dst stream. Error %d", err);

      printf("receiving data\n");
      // TODO: not sure about data type
      hcclErr = HcclRecv(actualDstPtr, size, HCCL_DATA_TYPE_INT8, srcRankId, comms[dstRankId], dstStream);
      printf("unlocekd recv\n");

      _mutex.unlock();
      printf("sync stream recv\n");

      if (hcclErr != HCCL_SUCCESS) HICR_THROW_RUNTIME("failed to receive data. Error %d", hcclErr);
      err = aclrtSynchronizeStream(dstStream);
      if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("failed to sync on receive data via hccl. Error %d", err);
      printf("data received\n");

      err = aclrtDestroyStream(dstStream);
      if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("failed to destroy stream in receive data hccl. Error %d", err);

      srcThread.join();

      source->increaseMessagesSent();
      destination->increaseMessagesRecv();

      return;
    }

    err = aclrtMemcpy(actualDstPtr, size, actualSrcPtr, size, memcpyKind);
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not copy memory from device. Error %d", err);

    source->increaseMessagesSent();
    destination->increaseMessagesRecv();
  }

  __USED__ static inline void sendDataHccl(HiCR::backend::ascend::Ascend *backend, deviceIdentifier_t srcId, void *ptr, size_t size, deviceIdentifier_t dstId, std::reference_wrapper<std::mutex> _mutex)
  {
    aclrtStream stream;
    aclError err;
    HcclResult hcclErr;

    // backend->selectDevice(srcId);
    // uint32_t srcRankId, srcRankSize;
    // hcclErr = HcclGetRankId(backend->comms[srcId], &srcRankId);
    // if (hcclErr != HCCL_SUCCESS) HICR_THROW_RUNTIME("Can not get source rank id in communication. Error %d", hcclErr);
    // hcclErr = HcclGetRankSize(backend->comms[srcId], &srcRankSize);
    // if (hcclErr != HCCL_SUCCESS) HICR_THROW_RUNTIME("Can not get source rank size in communication. Error %d", hcclErr);
    // printf("src dev id %ld rank id %d rank size %d\n", srcId, srcRankId, srcRankSize);

    _mutex.get().lock();
    printf("locekd send\n");

    backend->selectDevice(srcId);
    printf("ehi %d\n", backend->deviceCount);

    err = aclrtCreateStream(&stream);
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("failed to create src stream. Error %d", err);

    printf("sending data %ld %ld %ld\n", srcId, dstId, size);
    _mutex.get().unlock();
    hcclErr = HcclSend(ptr, size, HCCL_DATA_TYPE_INT8, dstId, backend->comms[srcId], stream);
    if (hcclErr != HCCL_SUCCESS) HICR_THROW_RUNTIME("failed to send data via hccl. Error %d", hcclErr);
    printf("unlocekd send\n");
    printf("sync stream send\n");
    err = aclrtSynchronizeStream(stream);
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("failed to sync stream on send data via hccl. Error %d", err);
    printf("data sent\n");

    err = aclrtDestroyStream(stream);
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("failed to destroy stream in send data hccl. Error %d", err);
  }

  __USED__ static inline void recvDataHccl(HiCR::backend::ascend::Ascend *backend, deviceIdentifier_t dstId, void *ptr, size_t size, deviceIdentifier_t srcId)
  {
    aclrtStream stream;
    aclError err;
    HcclResult hcclErr;

    backend->selectDevice(dstId);

    err = aclrtCreateStream(&stream);
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("failed to create dst stream. Error %d", err);

    printf("receiving data\n");
    // TODO: not sure about data type
    hcclErr = HcclRecv(ptr, size, HCCL_DATA_TYPE_INT8, srcId, backend->comms[0], stream);
    printf("sync stream recv\n");

    if (hcclErr != HCCL_SUCCESS) HICR_THROW_RUNTIME("failed to receive data. Error %d", hcclErr);
    err = aclrtSynchronizeStream(stream);
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("failed to sync on receive data via hccl. Error %d", err);
    printf("data received\n");

    err = aclrtDestroyStream(stream);
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("failed to destroy stream in receive data hccl. Error %d", err);
  }

  /**
   * Queries the backend to update the internal state of the memory slot.
   * One main use case of this function is to update the number of messages received and sent to/from this slot.
   * This is a non-blocking, non-collective function.
   *
   * \param[in] memorySlot Memory slot to query for updates.
   */
  __USED__ inline void queryMemorySlotUpdatesImpl(const MemorySlot *memorySlot) override
  {
    HICR_THROW_LOGIC("This function has not been implemented yet");
  }

  /**
   * Implementation of the fence operation for the ascend backend. In this case, nothing needs to be done, as
   * the memcpy operation is synchronous. This means that it's mere execution (whether immediate or deferred)
   * ensures its completion.
   */
  __USED__ inline void fenceImpl(const tag_t tag, const globalKeyToMemorySlotArrayMap_t &globalSlots) override
  {
    HICR_THROW_LOGIC("This function has not been implemented yet");
  }

  /**
   * Allocates memory in the current memory space (whole system)
   *
   * \param[in] memorySpace Memory space in which to perform the allocation.
   * \param[in] size Size of the memory slot to create
   * \returns The pointer of the newly allocated memory slot
   */
  __USED__ inline void *allocateLocalMemorySlotImpl(const memorySpaceId_t memorySpace, const size_t size) override
  {
    // do alloc on host
    // TODO: parametrize the host memoory space recognition

    void *ptr = NULL;
    if (memorySpace == _deviceStatusMap.size() - 1)
    {
      ptr = hostAlloc(memorySpace, size);
    }
    else
    {
      ptr = deviceAlloc(memorySpace, size);
    }
    // TODO: only for kernel execution
    // create data buffer for manage user data
    // aclDataBuffer *dataBuffer = aclCreateDataBuffer(ptr, size);
    // _memoryDataBufferMap[ptr] = dataBuffer;

    // keep track of the mapping between unique identifier and pointer + deviceId
    // TODO: change with unique memorySlotId
    if (_memoryAscendMap.count(ptr) != 0) HICR_THROW_RUNTIME("Pointer already allocated on host.");
    _memoryAscendMap[ptr] = globalAscendSlot_t{.deviceId = memorySpace, .pointer = ptr, .size = size};

    // TODO: update memory size?
    return ptr;
  }

  __USED__ inline void *hostAlloc(const memorySpaceId_t memorySpace, const size_t size)
  {
    void *ptr;

    // do the allocation on host memory
    aclError err = aclrtMallocHost(&ptr, size);
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not allocate memory on ascend host. Error %d", err);

    return ptr;
  }

  __USED__ inline void *deviceAlloc(const memorySpaceId_t memorySpace, const size_t size)
  {
    void *ptr;

    // select the device context on which we should allocate the memory
    selectDevice(memorySpace);

    // do the allocation on device memory
    aclError err = aclrtMalloc(&ptr, size, ACL_MEM_MALLOC_HUGE_FIRST_P2P);
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not allocate memory on ascend device %d. Error %d", memorySpace, err);

    return ptr;
  }

  /**
   * Associates a pointer locally-allocated manually and creates a local memory slot with it
   * \param[in] memorySlot The new local memory slot to register
   */
  __USED__ inline void registerLocalMemorySlotImpl(const MemorySlot *memorySlot) override
  {
    HICR_THROW_LOGIC("This function has not been implemented yet");
  }

  /**
   * De-registers a memory slot previously registered
   *
   * \param[in] memorySlot Memory slot to deregister.
   */
  __USED__ inline void deregisterLocalMemorySlotImpl(MemorySlot *memorySlot) override
  {
    HICR_THROW_LOGIC("This function has not been implemented yet");
  }

  __USED__ inline void deregisterGlobalMemorySlotImpl(MemorySlot *memorySlot) override
  {
    HICR_THROW_LOGIC("This function has not been implemented yet");
  }

  /**
   * Exchanges memory slots among different local instances of HiCR to enable global (remote) communication
   *
   * \param[in] tag Identifies a particular subset of global memory slots
   * \param[in] memorySlots Array of local memory slots to make globally accessible
   */
  __USED__ inline void exchangeGlobalMemorySlots(const tag_t tag, const std::vector<globalKeyMemorySlotPair_t> &memorySlots) override
  {
    HICR_THROW_LOGIC("This function has not been implemented yet");
  }

  /**
   * Backend-internal implementation of the freeLocalMemorySlot function
   *
   * \param[in] memorySlot Local memory slot to free up. It becomes unusable after freeing.
   */
  __USED__ inline void freeLocalMemorySlotImpl(MemorySlot *memorySlot) override
  {
    // Getting memory slot info
    const auto memorySlotPointer = memorySlot->getPointer();

    // TODO: change with memory slot id
    const auto memorySlotData = _memoryAscendMap.at(memorySlotPointer);

    if (memorySlotData.deviceId == _deviceStatusMap.size() - 1)
    {
      freeHostMemorySlot(memorySlotPointer);
    }
    else
    {
      freeDeviceMemorySlot(memorySlotData.deviceId, memorySlotData.pointer);
    }

    // aclError err;
    // err = aclDestroyDataBuffer(_memoryDataBufferMap.at(memorySlotPointer));
    // if (err != ACL_SUCCESS) HICR_THROW_LOGIC("Error destroying data buffer for memory slot %d on device %d. Error %d", memorySlot->getId(), memorySlotData.deviceId, err);

    // TODO: change with memory slot id
    // _memoryDataBufferMap.erase(memorySlotPointer);
    _memoryAscendMap.erase(memorySlotPointer);

    // TODO: update memory size?
  }

  /**
   * Release memory on the Host memory through Ascend-dedicated functions.
   *
   * \param[in] ptr Local memory to free up.
   */
  __USED__ inline void freeHostMemorySlot(const void *ptr)
  {
    aclError err;
    err = aclrtFreeHost((void *)ptr);
    if (err != ACL_SUCCESS) HICR_THROW_LOGIC("Error while freeing host memory. Error %d", err);
  }

  /**
   * Release memory on the Host memory through Ascend-dedicated functions.
   *
   * \param[in] deviceId Device id where memory is allocated.
   * \param[in] ptr Local memory to free up.
   */
  __USED__ inline void freeDeviceMemorySlot(const deviceIdentifier_t deviceId, const void *ptr)
  {
    selectDevice(deviceId);

    aclError err;
    err = aclrtFree((void *)ptr);
    if (err != ACL_SUCCESS) HICR_THROW_LOGIC("Error while freeing device %d memory. Error %d", deviceId, err);
  }

  /**
   * Backend-internal implementation of the isMemorySlotValid function
   *
   * \param[in] memorySlot Memory slot to check
   * \return True, if the referenced memory slot exists and is valid; false, otherwise
   */
  __USED__ bool isMemorySlotValidImpl(const MemorySlot *memorySlot) const override
  {
    // TODO: change with memory slot id
    const auto memorySlotPointer = memorySlot->getPointer();

    if (_memoryAscendMap.count(memorySlotPointer) == 0) return false;

    return true;
  }
};

} // namespace ascend
} // namespace backend
} // namespace HiCR
