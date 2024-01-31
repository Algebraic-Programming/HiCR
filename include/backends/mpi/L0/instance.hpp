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

#include <hicr/L0/instance.hpp>
#include <mpi.h>

namespace HiCR
{

namespace backend
{

namespace mpi
{

namespace L0
{

/**
 * This class represents an abstract definition for a HICR instance as represented by the MPI backend:
 */
class Instance final : public HiCR::L0::Instance
{
  public:

  /**
   * Constructor for a Instance class for the MPI backend
   * \param[in] rank The MPI rank corresponding to this HiCR instance
   */
  Instance(const int rank) : HiCR::L0::Instance((instanceId_t)rank), _rank(rank)
  {
  }

  /**
   * Default destructor
   */
  ~Instance() = default;

  virtual bool isRootInstance() const
  {
    // Criterion for root rank in MPI: Rank 0 will always be the root.
    return _rank == 0;
  };

  /**
   * Retrieves this HiCR Instance's MPI rank
   * \return The MPI rank corresponding to this instance
   */
  __USED__ inline int getRank() const { return _rank; }

  /**
   * Remembers the MPI rank this instance belongs to
   */
  const int _rank;
};

} // namespace L0

} // namespace mpi

} // namespace backend

} // namespace HiCR
