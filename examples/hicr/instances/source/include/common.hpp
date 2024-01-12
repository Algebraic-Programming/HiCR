#pragma once

#define TEST_RPC_PROCESSING_UNIT_ID 0
#define TEST_RPC_EXECUTION_UNIT_ID 0

#include <memory>
#include <nlohmann_json/json.hpp>
#include <hicr/L1/instanceManager.hpp>

#ifdef _HICR_USE_MPI_BACKEND_
#include <mpi.h>
#include <backends/host/hwloc/L1/topologyManager.hpp>
#include <backends/host/pthreads/L1/computeManager.hpp>
#include <backends/mpi/L1/memoryManager.hpp>
#include <backends/mpi/L1/instanceManager.hpp>
#endif // _HICR_USE_MPI_BACKEND_

// Selecting the appropriate instance manager based on the selected backends during compilation

namespace HiCR // Simulating this is part of HiCR for now
{

std::unique_ptr<HiCR::L1::InstanceManager> getInstanceManager(int* argc, char** argv[]);

#ifdef _HICR_USE_MPI_BACKEND_

std::unique_ptr<HiCR::L1::InstanceManager> getInstanceManager(int* argc, char** argv[])
{
  // Initializing MPI
  int requested = MPI_THREAD_SERIALIZED;
  int provided;
  MPI_Init_thread(argc, argv, requested, &provided);
  if (provided < requested) fprintf(stderr, "Warning, this example may not work properly if MPI does not support (serialized) threaded access\n");

  // Instantiating MPI communication manager
  auto cm = std::make_shared<HiCR::backend::mpi::L1::CommunicationManager>(MPI_COMM_WORLD);

  // Instantiating MPI memory manager
  auto mm = std::make_shared<HiCR::backend::mpi::L1::MemoryManager>();

  // Initializing host (CPU) compute manager manager
  auto km = std::make_shared<HiCR::backend::host::pthreads::L1::ComputeManager>();

  // Now instantiating the instance manager
  return std::make_unique<HiCR::backend::mpi::L1::InstanceManager>(cm, km, mm);
}

#endif // _HICR_USE_MPI_BACKEND_

} // namespace HiCR
