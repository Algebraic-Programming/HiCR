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
 * @file mpi/L0/globalMemorySlot.hpp
 * @brief Provides a definition for the global memory slot class for the MPI backend
 * @author S. M. Martin
 * @date 19/10/2023
 */
#pragma once

#include <memory>
#include <mpi.h>
#include <hicr/core/L0/localMemorySlot.hpp>
#include <hicr/core/L0/globalMemorySlot.hpp>
#include <utility>

namespace HiCR::backend::mpi::L0
{

/**
 * This class represents the  definition for a Global Memory Slot resource for the MPI backend:
 */
class GlobalMemorySlot final : public HiCR::L0::GlobalMemorySlot
{
  public:

  /**
   * Constructor for a MemorySlot class for the MPI backend
   *
   * \param[in] rank Rank to which this memory slot belongs
   * \param[in] globalTag For global memory slots, indicates the subset of global memory slots this belongs to
   * \param[in] globalKey Unique identifier for that memory slot that this slot occupies.
   * \param[in] sourceLocalMemorySlot The local memory slot (if applicable) from whence this global memory slot is created
   */
  GlobalMemorySlot(int                                           rank,
                   const HiCR::L0::GlobalMemorySlot::tag_t       globalTag             = 0,
                   const HiCR::L0::GlobalMemorySlot::globalKey_t globalKey             = 0,
                   std::shared_ptr<HiCR::L0::LocalMemorySlot>    sourceLocalMemorySlot = nullptr)
    : HiCR::L0::GlobalMemorySlot(globalTag, globalKey, std::move(sourceLocalMemorySlot)),
      _rank(rank)
  {}

  /**
   * Default destructor
   */
  ~GlobalMemorySlot() override = default;

  /**
   * Returns the rank to which this memory slot belongs
   *
   * \return The rank to which this memory slot belongs
   */
  __INLINE__ const int getRank() { return _rank; }

  /**
   * If this is a global slot, it returns a pointer to the MPI window for the actual memory slot data
   *
   * \return A pointer to the MPI window for the actual memory slot data
   */
  __INLINE__ std::unique_ptr<MPI_Win> &getDataWindow() { return _dataWindow; }

  /**
   * If this is a global slot, it returns a pointer to the MPI window for the received message count
   *
   * \return A pointer to the MPI window for the received message count
   */
  __INLINE__ std::unique_ptr<MPI_Win> &getRecvMessageCountWindow() { return _recvMessageCountWindow; }

  /**
   * If this is a global slot, it returns a pointer to the MPI window for the sent message count
   *
   * \return A pointer to the MPI window for the sent message count
   */
  __INLINE__ std::unique_ptr<MPI_Win> &getSentMessageCountWindow() { return _sentMessageCountWindow; }

  /**
   * Returns whether the memory slot lock has been acquired by the current MPI instance
   *
   * @return The internal state of _lockAcquired
   */
  [[nodiscard]] __INLINE__ bool getLockAcquiredValue() const { return _lockAcquired; }

  /**
   * Sets memory slot lock state (whether it has been acquired by the current MPI instance or not)
   *
   * @param[in] value The internal state of _lockAcquired to set
   */
  __INLINE__ void setLockAcquiredValue(const bool value) { _lockAcquired = value; }

  private:

  /**
   * Indicates whether we hold a lock on the current slot's windows
   */
  bool _lockAcquired = false;

  /**
   * Remembers the MPI rank this memory slot belongs to
   */
  int _rank;

  /**
   * Stores the MPI window to use with this slot to move the actual data
   */
  std::unique_ptr<MPI_Win> _dataWindow = nullptr;

  /**
   * Stores the MPI window to use with this slot to update received message count
   */
  std::unique_ptr<MPI_Win> _recvMessageCountWindow = nullptr;

  /**
   * Stores the MPI window to use with this slot to update sent message count
   */
  std::unique_ptr<MPI_Win> _sentMessageCountWindow = nullptr;
};

} // namespace HiCR::backend::mpi::L0
