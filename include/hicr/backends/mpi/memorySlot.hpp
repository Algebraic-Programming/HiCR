/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file memorySlot.hpp
 * @brief Provides a definition for the memory slot class for the shared Memory backend
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


  MemorySlot(
    int rank,
    void *const pointer,
    const size_t size,
    const tag_t globalTag = 0,
    const globalKey_t globalKey = 0) :
     HiCR::MemorySlot(pointer, size, globalTag, globalKey),
     _rank(rank)
  {
  }

  ~MemorySlot() = default;

  int getRank() { return _rank; }
  MPI_Win*& getDataWindow() { return _dataWindow; }
  MPI_Win*& getRecvMessageCountWindow() { return _recvMessageCountWindow; }
  MPI_Win*& getSentMessageCountWindow() { return _sentMessageCountWindow; }

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

