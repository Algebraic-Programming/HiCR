#include <lpf/core.h>
#include <lpf/mpi.h>
#include <hicr/backends/lpf/L1/memoryManager.hpp>
#include <hicr/backends/lpf/L1/communicationManager.hpp>
#include <hicr/backends/mpi/L1/instanceManager.hpp>
#include <hicr/backends/hwloc/L1/topologyManager.hpp>

#include <hicr/frontends/objectStore/objectStore.hpp>

#include "include/ownerInstance.hpp"
#include "include/readerInstance.hpp"
#include "include/common.hpp"

// flag needed when using MPI to launch
const int LPF_MPI_AUTO_INITIALIZE = 0;

/**
 * #DEFAULT_MEMSLOTS The memory slots used by LPF
 * in lpf_resize_memory_register . This value is currently
 * guessed as sufficiently large for a program
 */
#define DEFAULT_MEMSLOTS 100

/**
 * #DEFAULT_MSGSLOTS The message slots used by LPF
 * in lpf_resize_message_queue . This value is currently
 * guessed as sufficiently large for a program
 */
#define DEFAULT_MSGSLOTS 100

typedef struct my_args
{
  int    argc;
  char **argv;
} my_args_t;

void spmd(lpf_t lpf, lpf_pid_t pid, lpf_pid_t nprocs, lpf_args_t args)
{
  my_args_t                                 *my_args         = (my_args_t *)args.input;
  std::unique_ptr<HiCR::L1::InstanceManager> instanceManager = HiCR::backend::mpi::L1::InstanceManager::createDefault(&my_args->argc, &my_args->argv);
  auto                                       instanceId      = instanceManager->getCurrentInstance()->getId();

  // Initializing LPF
  CHECK(lpf_resize_message_queue(lpf, DEFAULT_MSGSLOTS));
  CHECK(lpf_resize_memory_register(lpf, DEFAULT_MEMSLOTS));
  CHECK(lpf_sync(lpf, LPF_SYNC_DEFAULT));

  auto communicationManager = std::make_unique<HiCR::backend::lpf::L1::CommunicationManager>(nprocs, pid, lpf);
  auto memoryManager        = std::make_unique<HiCR::backend::lpf::L1::MemoryManager>(lpf);

  // Using HWLoc as topology manager
  hwloc_topology_t topology;
  hwloc_topology_init(&topology);
  auto hwlocTopologyManager = std::make_unique<HiCR::backend::hwloc::L1::TopologyManager>(&topology);

  // Asking backend to check the available devices
  const auto t = hwlocTopologyManager->queryTopology();

  // Getting first device found
  auto d = *t.getDevices().begin();

  // Obtaining memory spaces
  auto memSpaces = d->getMemorySpaceList();

  // Getting a reference to the first memory space
  auto firstMemorySpace = *memSpaces.begin();

  auto objectStoreManager = HiCR::objectStore::ObjectStore(*communicationManager, OBJECT_STORE_TAG, *memoryManager, firstMemorySpace, instanceId);

  if (pid == 0) { owner(*memoryManager, *communicationManager, objectStoreManager); }
  else { reader(*memoryManager, *communicationManager, objectStoreManager); }

  // Destroying topology
  hwloc_topology_destroy(topology);

  /**
   * We cannot call finalize here, because
   * it calls MPI_Finalize. However, lpf_hook performs
   * MPI_Allreduce after completing the spmd function.
   * The MPI runtime will therefore crash if we do:
   * instanceManager->finalize() in spmd called from lpf_hook,
   * as in effect, we will have a sequence
   * MPI_Finalize();
   * MPI_Allreduce(...);
   *
   */

  //instanceManager->finalize();
}

int main(int argc, char **argv)
{
  my_args_t  my_args{argc, argv};
  lpf_args_t args;
  memset(&args, 0, sizeof(lpf_args_t));
  args.input       = &my_args;
  args.input_size  = sizeof(my_args_t);
  args.output      = nullptr;
  args.output_size = 0;
  args.f_size      = 0;
  args.f_symbols   = nullptr;

  MPI_Init(&argc, &argv);
  lpf_init_t init;
  CHECK(lpf_mpi_initialize_with_mpicomm(MPI_COMM_WORLD, &init));
  CHECK(lpf_hook(init, &spmd, args));
  CHECK(lpf_mpi_finalize(init));
  MPI_Finalize();

  return 0;
}
