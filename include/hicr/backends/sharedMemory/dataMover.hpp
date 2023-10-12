/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file dataMover.hpp
 * @brief This file provides support for data motion operations in the shared memory backend
 * @author S. M. Martin
 * @date 14/8/2023
 */

#pragma once

#include "hwloc.h"
#include <hicr/backends/dataMover.hpp>

namespace HiCR
{

namespace backend
{

namespace sharedMemory
{

/**
 * Implementation of the data motion support for the shared memory backend
 */
class DataMover final : public backend::DataMover
{
  public:

  /**
   * The constructor is employed to create the barriers required to coordinate threads
   *
   * \param[in] fenceCount Specifies how many times a fence has to be called for it to release callers
   */
  DataMover(const size_t fenceCount = 1) : backend::DataMover()
  {
    // Initializing barrier for fence operation
    pthread_barrier_init(&_fenceBarrier, NULL, fenceCount);
  }

  /**
   * The constructor is employed to free memory required for hwloc
   */
  ~DataMover()
  {
    // Freeing barrier memory
    pthread_barrier_destroy(&_fenceBarrier);
  }

  private:

  /**
   * Stores a barrier object to check on a fence operation
   */
  pthread_barrier_t _fenceBarrier;

  /**
   * Implementation of the fence operation for the shared memory backend. In this case, nothing needs to be done, as
   * the system's memcpy operation is synchronous. This means that it's mere execution (whether immediate or deferred)
   * ensures its completion.
   */
  __USED__ inline void fenceImpl(const tag_t tag) override
  {
    pthread_barrier_wait(&_fenceBarrier);
  }

  __USED__ inline void memcpyImpl(MemorySlot *destination, const size_t dst_offset, MemorySlot *source, const size_t src_offset, const size_t size) override
  {
    // Getting slot pointers
    const auto srcPtr = source->getPointer();
    const auto dstPtr = destination->getPointer();

    // Calculating actual offsets
    const auto actualSrcPtr = (void *)((uint8_t *)srcPtr + src_offset);
    const auto actualDstPtr = (void *)((uint8_t *)dstPtr + dst_offset);

    // Running the requested operation
    std::memcpy(actualDstPtr, actualSrcPtr, size);

    // Increasing message received/sent counters for memory slots
    source->increaseMessagesSent();
    destination->increaseMessagesRecv();
  }
};

} // namespace sharedMemory
} // namespace backend
} // namespace HiCR
