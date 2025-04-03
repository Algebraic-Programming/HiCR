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
 * @file instance.hpp
 * @brief Provides a definition for the instance class for the MPI backend
 * @author S. M. Martin
 * @date 2/11/2023
 */
#pragma once

#include <mpi.h>
#include <hicr/core/L0/instance.hpp>

namespace HiCR::backend::mpi::L0
{

/**
 * Definition for the root rank for an MPI deployment (zero because rank zero is always present)
 */
constexpr int _HICR_MPI_INSTANCE_ROOT_ID = 0;

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
  Instance(const int rank)
    : HiCR::L0::Instance((instanceId_t)rank),
      _rank(rank)
  {}

  /**
   * Default destructor
   */
  ~Instance() override = default;

  [[nodiscard]] bool isRootInstance() const override
  {
    // Criterion for root rank in MPI: Rank 0 will always be the root.
    return _rank == _HICR_MPI_INSTANCE_ROOT_ID;
  };

  /**
   * Retrieves this HiCR Instance's MPI rank
   * \return The MPI rank corresponding to this instance
   */
  [[nodiscard]] __INLINE__ int getRank() const { return _rank; }

  /**
   * Remembers the MPI rank this instance belongs to
   */
  const int _rank;
};

} // namespace HiCR::backend::mpi::L0
