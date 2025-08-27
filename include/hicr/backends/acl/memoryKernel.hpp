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
 * @file memoryKernel.hpp
 * @brief This file implements the kernel class for the acl backend
 * @author S. M. Martin & L. Terracciano
 * @date 13/11/2023
 */

#pragma once

#include <acl/acl.h>
#include <hicr/core/exceptions.hpp>
#include <hicr/core/localMemorySlot.hpp>
#include <hicr/backends/acl/communicationManager.hpp>
#include <hicr/backends/acl/kernel.hpp>

namespace HiCR::backend::acl
{ /**
 * This class represents a replicable Memory Kernel for the acl backend.
 * A Memory Kernel enable the execution of memcopy operations in a stream/sequence of Kernels.
 * Memory Kernels currently supports memcpy operations on the same device, since they are meant to be chained
 * with other Kernels.
 */
class MemoryKernel final : public Kernel
{
  public:

  /**
   * Constructor for the execution unit class of the acl backend
   *
   * \param commManager the acl communication manager
   * \param destination destination pointer
   * \param destinationOffset destination offset
   * \param source source pointer
   * \param sourceOffset source offset
   * \param size the number of bytes to copy
   */
  MemoryKernel(acl::CommunicationManager          *commManager,
               std::shared_ptr<HiCR::LocalMemorySlot> destination,
               const size_t                           destinationOffset,
               std::shared_ptr<HiCR::LocalMemorySlot> source,
               const size_t                           sourceOffset,
               size_t                                 size)
    : acl::Kernel(),
      _dst(destination),
      _src(source),
      _dstOffset(destinationOffset),
      _srcOffset(sourceOffset),
      _size(size),
      _commManager(commManager){};

  MemoryKernel() = delete;

  /**
   * Default destructor
   */
  ~MemoryKernel() {}

  /**
   * Execute the memcpy on the \p stream . The operation is just enqueued on the stream, that is, it is async
   *
   * \param stream ACL stream on which memcpy is executed
   */
  __INLINE__ void start(const aclrtStream stream) override { _commManager->memcpyAsync(_dst, _dstOffset, _src, _srcOffset, _size, stream); }

  private:

  /**
   * Destionation memory slot
   */
  const std::shared_ptr<HiCR::LocalMemorySlot> _dst;
  /**
   * Source memory slot
   */
  const std::shared_ptr<HiCR::LocalMemorySlot> _src;

  /**
   * Destination offset
   */
  const size_t _dstOffset;

  /**
   * Source offset
   */
  const size_t _srcOffset;
  /**
   * Data size to be copied
   */
  const size_t _size;

  /**
   * acl memory manager
   */
  acl::CommunicationManager *const _commManager;
};
} // namespace HiCR::backend::acl
