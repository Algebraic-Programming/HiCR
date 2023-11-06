/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file instance.hpp
 * @brief Provides a definition for the instance class for the MPI backend
 * @author S. M. Martin
 * @date 2/11/2023
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
 * This class represents an abstract definition for a HICR instance as represented by the MPI backend:
 */
class Instance final : public HiCR::Instance
{
  public:

  /**
   * Constructor for a Instance class for the MPI backend
   */
  Instance(int rank) : _rank(rank)
  {
  }

  /**
   * Default destructor
   */
  ~Instance() = default;

  __USED__ inline int getRank() const { return _rank; }

  private:

  /**
   * Remembers the MPI rank this instance belongs to
   */
  int _rank;
};

} // namespace mpi

} // namespace backend

} // namespace HiCR
