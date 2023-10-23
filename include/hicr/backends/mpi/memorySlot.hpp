/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file memorySlot.hpp
 * @brief Provides a definition for the memory slot class for the MPI backend
 * @author S. M. Martin
 * @date 19/10/2023
 */
#pragma once

#include <hicr/memorySlot.hpp>

namespace HiCR
{

namespace backend
{

namespace mpi
{

/**
 * This class represents an abstract definition for a Memory Slot resource in HiCR that:
 *
 * - Represents a contiguous segment within a memory space, with a starting address and a size
 */
class MemorySlot final : public HiCR::MemorySlot
{
  public:

  /**
   * Constructor for a MemorySlot class for the MPI backend
   *
   * \param[in] rank Rank to which this memory slot belongs
   * \param[in] pointer If this is a local slot (same rank as this the running process), this pointer indicates the address of the local memory segment
   * \param[in] size The size (in bytes) of the memory slot, assumed to be contiguous
   * \param[in] globalTag For global memory slots, indicates the subset of global memory slots this belongs to
   * \param[in] globalKey Unique identifier for that memory slot that this slot occupies.
   */
  MemorySlot(
    int rank,
    void *const pointer,
    const size_t size,
    const tag_t globalTag = 0,
    const globalKey_t globalKey = 0) : HiCR::MemorySlot(pointer, size, globalTag, globalKey),
                                       _rank(rank)
  {
  }

  /**
   * Default destructor
   */
  ~MemorySlot() = default;

  /**
   * Returns the rank to which this memory slot belongs
   *
   * \return The rank to which this memory slot belongs
   */
  const int getRank() { return _rank; }

  /**
   * If this is a global slot, it returns a pointer to the MPI window for the actual memory slot data
   *
   * \return A pointer to the MPI window for the actual memory slot data
   */
  MPI_Win *&getDataWindow() { return _dataWindow; }

  /**
   * If this is a global slot, it returns a pointer to the MPI window for the received message count
   *
   * \return A pointer to the MPI window for the received message count
   */
  MPI_Win *&getRecvMessageCountWindow() { return _recvMessageCountWindow; }

  /**
   * If this is a global slot, it returns a pointer to the MPI window for the sent message count
   *
   * \return A pointer to the MPI window for the sent message count
   */
  MPI_Win *&getSentMessageCountWindow() { return _sentMessageCountWindow; }

  private:

  /**
   * Remembers the MPI rank this memory slot belongs to
   */
  int _rank;

  /**
   * Stores the MPI window to use with this slot to move the actual data
   */
  MPI_Win *_dataWindow = NULL;

  /**
   * Stores the MPI window to use with this slot to update received message count
   */
  MPI_Win *_recvMessageCountWindow = NULL;

  /**
   * Stores the MPI window to use with this slot to update sent message count
   */
  MPI_Win *_sentMessageCountWindow = NULL;
};

} // namespace mpi

} // namespace backend

} // namespace HiCR
