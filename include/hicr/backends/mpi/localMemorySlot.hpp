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
 * @file localMemorySlot.hpp
 * @brief Provides a definition for the local memory slot class for the MPI backend
 * @author S. M. Martin
 * @date 19/10/2023
 */
#pragma once

#include <hicr/core/localMemorySlot.hpp>
#include <utility>

namespace HiCR::backend::mpi
{
class CommunicationManager;
}

namespace HiCR::backend::mpi
{

/**
 * This class represents an abstract definition for a Memory Slot resource in HiCR that:
 *
 * - Represents a contiguous segment within a memory space, with a starting address and a size
 */
class LocalMemorySlot final : public HiCR::LocalMemorySlot
{
  friend class HiCR::backend::mpi::CommunicationManager;

  public:

  /**
   * Constructor for a MemorySlot class for the MPI backend
   *
   * \param[in] pointer If this is a local slot (same rank as this the running process), this pointer indicates the address of the local memory segment
   * \param[in] size The size (in bytes) of the memory slot, assumed to be contiguous
   * \param[in] memorySpace The memory space from whence this memory slot was created
   */
  LocalMemorySlot(void *const pointer, const size_t size, std::shared_ptr<HiCR::MemorySpace> memorySpace)
    : HiCR::LocalMemorySlot(pointer, size, std::move(memorySpace))
  {}

  ~LocalMemorySlot() override = default;
};

} // namespace HiCR::backend::mpi
