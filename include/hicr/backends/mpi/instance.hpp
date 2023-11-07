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

#include <mpi.h>

namespace HiCR
{

namespace backend
{

namespace mpi
{

#define _HICR_MPI_INSTANCE_ROOT_RANK 0

// The base tag can be changed if it collides with others
#ifndef _HICR_MPI_INSTANCE_BASE_TAG
#define _HICR_MPI_INSTANCE_BASE_TAG 4096
#endif
#define _HICR_MPI_INSTANCE_PROCESSING_UNIT_TAG (_HICR_MPI_INSTANCE_BASE_TAG+1)
#define _HICR_MPI_INSTANCE_EXECUTION_UNIT_TAG  (_HICR_MPI_INSTANCE_BASE_TAG+2)


/**
 * This class represents an abstract definition for a HICR instance as represented by the MPI backend:
 */
class Instance final : public HiCR::Instance
{
  public:

  /**
   * Constructor for a Instance class for the MPI backend
   */
  Instance(const int rank, const MPI_Comm comm) : _rank(rank), _comm(comm)
  {
  }

  /**
   * Default destructor
   */
  ~Instance() = default;

  __USED__ inline int getRank() const { return _rank; }

  /**
   * Function to invoke the execution of a remote function in a remote HiCR instance
   */
  __USED__ inline void invoke(const processingUnitIndex_t pIdx, const executionUnitIndex_t eIdx) override
  {
   // Getting rank Id for the passed instance
   const auto dest = getRank();

   // Sending request
   MPI_Send(&eIdx, 1, MPI_UNSIGNED_LONG, dest, _HICR_MPI_INSTANCE_EXECUTION_UNIT_TAG, _comm);
   MPI_Send(&pIdx, 1, MPI_UNSIGNED_LONG, dest, _HICR_MPI_INSTANCE_PROCESSING_UNIT_TAG, _comm);
  }

  private:

  /**
   * Remembers the MPI rank this instance belongs to
   */
  const int _rank;

  /**
   * Remembers the MPI communicator this rank belongs to
   */
  const MPI_Comm _comm;
};

} // namespace mpi

} // namespace backend

} // namespace HiCR
