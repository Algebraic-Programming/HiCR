/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
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
#include <hicr/L0/localMemorySlot.hpp>
#include <hicr/L0/globalMemorySlot.hpp>

namespace HiCR
{

namespace backend
{

namespace mpi
{

namespace L0
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
  GlobalMemorySlot(
    int rank,
    const HiCR::L0::GlobalMemorySlot::tag_t globalTag = 0,
    const HiCR::L0::GlobalMemorySlot::globalKey_t globalKey = 0,
    std::shared_ptr<HiCR::L0::LocalMemorySlot> sourceLocalMemorySlot = nullptr) : HiCR::L0::GlobalMemorySlot(globalTag, globalKey, sourceLocalMemorySlot),
                                                                                  _rank(rank)
  {
  }

  /**
   * Default destructor
   */
  ~GlobalMemorySlot() = default;

  /**
   * Returns the rank to which this memory slot belongs
   *
   * \return The rank to which this memory slot belongs
   */
  __USED__ inline const int getRank() { return _rank; }

  /**
   * If this is a global slot, it returns a pointer to the MPI window for the actual memory slot data
   *
   * \return A pointer to the MPI window for the actual memory slot data
   */
  __USED__ inline std::unique_ptr<MPI_Win> &getDataWindow() { return _dataWindow; }

  /**
   * If this is a global slot, it returns a pointer to the MPI window for the received message count
   *
   * \return A pointer to the MPI window for the received message count
   */
  __USED__ inline std::unique_ptr<MPI_Win> &getRecvMessageCountWindow() { return _recvMessageCountWindow; }

  /**
   * If this is a global slot, it returns a pointer to the MPI window for the sent message count
   *
   * \return A pointer to the MPI window for the sent message count
   */
  __USED__ inline std::unique_ptr<MPI_Win> &getSentMessageCountWindow() { return _sentMessageCountWindow; }

  /**
   * Returns whether the memory slot lock has been acquired by the current MPI instance
   *
   * @return The internal state of _lockAcquired
   */
  __USED__ inline bool getLockAcquiredValue() const { return _lockAcquired; }

  /**
   * Sets memory slot lock state (whether it has been acquired by the current MPI instance or not)
   *
   * @param[in] value The internal state of _lockAcquired to set
   */
  __USED__ inline void setLockAcquiredValue(const bool value) { _lockAcquired = value; }

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
  std::unique_ptr<MPI_Win> _dataWindow = NULL;

  /**
   * Stores the MPI window to use with this slot to update received message count
   */
  std::unique_ptr<MPI_Win> _recvMessageCountWindow = NULL;

  /**
   * Stores the MPI window to use with this slot to update sent message count
   */
  std::unique_ptr<MPI_Win> _sentMessageCountWindow = NULL;
};

} // namespace L0

} // namespace mpi

} // namespace backend

} // namespace HiCR
