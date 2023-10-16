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

namespace sequential
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
  DataMover(const size_t fenceCount = 1) : backend::DataMover(), _fenceCount {fenceCount} {}
  ~DataMover() = default;

  private:

  /**
   * Specifies how many times a fence has to be called for it to release callers
   */
  const size_t _fenceCount;

  /**
   * Common definition of a collection of memory slots
   */
  typedef std::map<tag_t, size_t> fenceCountTagMap_t;

  /**
   * Counter for calls to fence, filtered per tag
   */
  fenceCountTagMap_t _fenceCountTagMap;

  /**
   * Implementation of the fence operation for the shared memory backend. In this case, nothing needs to be done, as
   * the system's memcpy operation is synchronous. This means that it's mere execution (whether immediate or deferred)
   * ensures its completion.
   */
  __USED__ inline void fenceImpl(const tag_t tag) override
  {
   // Increasing the counter for the fence corresponding to the tag
   _fenceCountTagMap[tag]++;

   // Until we reached the required count, wait on it
   while (_fenceCountTagMap[tag] % _fenceCount != 0)
     ;
  }

  __USED__ inline void memcpyImpl(MemorySlot *destination, const size_t dst_offset, MemorySlot *source, const size_t src_offset, const size_t size) override
  {
   // Getting slot pointers
   const auto srcPtr = source->getPointer();
   const auto dstPtr = destination->getPointer();

   // Calculating actual offsets
   const auto actualSrcPtr = (void *)((uint8_t *)srcPtr + src_offset);
   const auto actualDstPtr = (void *)((uint8_t *)dstPtr + dst_offset);

   // Running memcpy now
   std::memcpy(actualDstPtr, actualSrcPtr, size);

   // Increasing message received/sent counters for memory slots
   source->increaseMessagesSent();
   destination->increaseMessagesRecv();
  }
};

} // namespace sequential
} // namespace backend
} // namespace HiCR
