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
 * @file acl/localMemorySlot.hpp
 * @brief Provides a definition for the local memory slot class for the acl backend
 * @author L. Terracciano & S. M. Martin
 * @date 19/10/2023
 */
#pragma once

#include <acl/acl.h>
#include <hicr/core/localMemorySlot.hpp>
#include <hicr/core/memorySpace.hpp>

namespace HiCR::backend::acl
{

/**
 * This class represents an abstract definition for a Local Memory Slot resource for the acl backend
 */
class LocalMemorySlot final : public HiCR::LocalMemorySlot
{
  public:

  /**
   * Constructor for a MemorySlot class for the acl backend
   *
   * \param pointer if this is a local slot (same rank as this the running process), this pointer indicates the address of the local memory segment
   * \param size the size of the memory slot, assumed to be contiguous
   * \param dataBuffer the ACL data buffer created for the memory slot
   * \param memorySpace the Huawei device memory from which this memory slot was obtained
   */
  LocalMemorySlot(void *const pointer, size_t size, const aclDataBuffer *dataBuffer, std::shared_ptr<HiCR::MemorySpace> memorySpace)
    : HiCR::LocalMemorySlot(pointer, size, memorySpace),
      _dataBuffer(dataBuffer){};

  /**
   * Default destructor
   */
  ~LocalMemorySlot() = default;

  /**
   * Return the ACL data buffer associated to the memory slot
   *
   * \return the ACL data buffer associated to the memory slot
   */
  __INLINE__ const aclDataBuffer *getDataBuffer() const { return _dataBuffer; }

  private:

  /**
   * The acl Data Buffer associated with the memory slot
   */
  const aclDataBuffer *_dataBuffer;
};

} // namespace HiCR::backend::acl
