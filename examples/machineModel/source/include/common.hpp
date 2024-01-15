#pragma once

#define PROCESSING_UNIT_ID 0

#define FINALIZATION_EXECUTION_UNIT_ID 0
#define TOPOLOGY_EXECUTION_UNIT_ID 1
#define TASK_A_EXECUTION_UNIT_ID 2
#define TASK_B_EXECUTION_UNIT_ID 3
#define TASK_C_EXECUTION_UNIT_ID 4


#include <memory>
#include <fstream>
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

std::unique_ptr<HiCR::L1::InstanceManager> initialize(int* argc, char** argv[]);
void finalize();

#ifdef _HICR_USE_MPI_BACKEND_

std::unique_ptr<HiCR::L1::InstanceManager> initialize(int* argc, char** argv[])
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

void finalize()
{
  MPI_Finalize();
}

#endif // _HICR_USE_MPI_BACKEND_

} // namespace HiCR


// Taken from https://stackoverflow.com/questions/116038/how-do-i-read-an-entire-file-into-a-stdstring-in-c/116220#116220
inline std::string slurp(std::ifstream &in)
{
  std::ostringstream sstr;
  sstr << in.rdbuf();
  return sstr.str();
}

// Loads a string from a given file
inline bool loadStringFromFile(std::string &dst, const std::string fileName)
{
  std::ifstream fi(fileName);

  // If file not found or open, return false
  if (fi.good() == false) return false;

  // Reading entire file
  dst = slurp(fi);

  // Closing file
  fi.close();

  return true;
}