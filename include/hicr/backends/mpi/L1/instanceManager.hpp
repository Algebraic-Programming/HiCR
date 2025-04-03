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
 * @file instanceManager.hpp
 * @brief This file implements the instance manager class for the MPI backend
 * @author S. M. Martin
 * @date 2/11/2023
 */

#pragma once

#include <memory>
#include <mpi.h>
#include <hicr/core/definitions.hpp>
#include <hicr/core/L1/instanceManager.hpp>
#include <hicr/backends/mpi/L0/instance.hpp>

namespace HiCR::backend::mpi::L1
{

#ifndef _HICR_MPI_INSTANCE_BASE_TAG
  /**
   * Base instance tag for data passing
   *
   * The base tag can be changed if it collides with others
   */
  #define _HICR_MPI_INSTANCE_BASE_TAG 4096
#endif

/**
 * Tag to communicate an RPC's target
 */
#define _HICR_MPI_RPC_TAG (_HICR_MPI_INSTANCE_BASE_TAG + 1)

/**
 * Tag to communicate an RPC's result size information
 */
#define _HICR_MPI_INSTANCE_RETURN_SIZE_TAG (_HICR_MPI_INSTANCE_BASE_TAG + 2)

/**
 * Tag to communicate an RPC's result data
 */
#define _HICR_MPI_INSTANCE_RETURN_DATA_TAG (_HICR_MPI_INSTANCE_BASE_TAG + 3)

/**
 * Implementation of the HiCR MPI Instance Manager
 */
class InstanceManager final : public HiCR::L1::InstanceManager
{
  public:

  /**
   * Constructor for the MPI instance manager
   *
   * \param[in] comm The MPI subcommunicator to use in instance detection and communication
   */
  InstanceManager(MPI_Comm comm)
    : HiCR::L1::InstanceManager(),
      _comm(comm)
  {
    // Getting current rank within and size of communicator
    MPI_Comm_size(_comm, &_size);
    MPI_Comm_rank(_comm, &_rank);

    // In MPI, the initial set of processes represents all the currently available instances of HiCR
    for (int i = 0; i < _size; i++)
    {
      // Creating new MPI-based HiCR instance
      auto instance = std::make_shared<HiCR::backend::mpi::L0::Instance>(i);

      // If this is the current rank, set it as current instance
      if (i == _rank) setCurrentInstance(instance);

      // Adding instance to the collection
      addInstance(instance);
    }
  }

  ~InstanceManager() override = default;

  __INLINE__ void finalize() override { MPI_Finalize(); }

  __INLINE__ void abort(int errorCode) override { MPI_Abort(MPI_COMM_WORLD, errorCode); }

  /**
   * This function represents the default intializer for this backend
   *
   * @param[in] argc A pointer to the argc value, as passed to main() necessary for new HiCR instances to count with argument information upon creation.
   * @param[in] argv A pointer to the argv value, as passed to main() necessary for new HiCR instances to count with argument information upon creation.
   * @return A unique pointer to the newly instantiated backend class
   */
  __INLINE__ static std::unique_ptr<HiCR::L1::InstanceManager> createDefault(int *argc, char ***argv)
  {
    // Initializing MPI
    int initialized = 0;
    MPI_Initialized(&initialized);
    if (initialized == 0)
    {
      int requested = MPI_THREAD_SINGLE;
      int provided  = 0;
      MPI_Init_thread(argc, argv, requested, &provided);
      if (provided < requested) fprintf(stderr, "Warning, your application may not work properly if MPI does not support  threaded access\n");
    }

    // Setting MPI_COMM_WORLD error handler
    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

    // Creating instance manager
    return std::make_unique<HiCR::backend::mpi::L1::InstanceManager>(MPI_COMM_WORLD);
  }

  [[nodiscard]] __INLINE__ HiCR::L0::Instance::instanceId_t getRootInstanceId() const override { return HiCR::backend::mpi::L0::_HICR_MPI_INSTANCE_ROOT_ID; }

  protected:

  __INLINE__ std::shared_ptr<HiCR::L0::Instance> createInstanceImpl [[noreturn]] (const std::shared_ptr<HiCR::L0::InstanceTemplate> &instanceTemplate) override
  {
    HICR_THROW_LOGIC("The MPI backend does not currently support the launching of new instances during runtime");
  }

  __INLINE__ std::shared_ptr<HiCR::L0::Instance> addInstanceImpl(HiCR::L0::Instance::instanceId_t instanceId) override
  {
    HICR_THROW_LOGIC("The Host backend does not currently support the detection of new instances during runtime");
  }

  private:

  /**
   * This value remembers what is the MPI rank of the instance that requested the execution of an RPC
   */
  int _RPCRequestRank = 0;

  /**
   * Default MPI communicator to use for this backend
   */
  const MPI_Comm _comm;

  /**
   * Number of MPI processes in the communicator
   */
  int _size{};

  /**
   * MPI rank corresponding to this process
   */
  int _rank{};
};

} // namespace HiCR::backend::mpi::L1
