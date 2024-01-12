#include <mpi.h>
#include <backends/host/hwloc/L1/topologyManager.hpp>
#include <backends/host/pthreads/L1/computeManager.hpp>
#include <backends/mpi/L1/memoryManager.hpp>
#include <backends/mpi/L1/instanceManager.hpp>

int main(int argc, char **argv)
{
  // Creating HWloc topology object
  hwloc_topology_t topology;

  // Reserving memory for hwloc
  hwloc_topology_init(&topology);

  // Initializing MPI
  int requested = MPI_THREAD_SERIALIZED;
  int provided;
  MPI_Init_thread(&argc, &argv, requested, &provided);
  if (provided < requested) fprintf(stderr, "Warning, this example may not work properly if MPI does not support (serialized) threaded access\n");

  // Instantiating MPI communication manager
  HiCR::backend::mpi::L1::CommunicationManager cm(MPI_COMM_WORLD);

  // Instantiating MPI memory manager
  HiCR::backend::mpi::L1::MemoryManager mm;
  
  // Initializing host (CPU) topology manager
  HiCR::backend::host::hwloc::L1::TopologyManager tm(&topology);

  // Initializing host (CPU) compute manager manager
  HiCR::backend::host::pthreads::L1::ComputeManager km;

  // Asking backend to check the available devices
  const auto t = tm.queryTopology();

  // Getting first device found
  auto d = *t.getDevices().begin();

  // Obtaining memory spaces
  auto memSpaces = d->getMemorySpaceList();

  // Selecting a memory space to allocate the required buffers into
  auto bufferMemSpace = *memSpaces.begin();

  // Now instantiating the instance manager
  auto im = HiCR::backend::mpi::L1::InstanceManager(cm, km, mm, bufferMemSpace);

  // Get the locally running instance
  auto myInstance = im.getCurrentInstance();

  // Bifurcating paths based on whether the instance is root or not
  if (myInstance->isRootInstance() == true)  printf("I am root\n");
  if (myInstance->isRootInstance() == false) printf("I am not root\n");

  // Finalizing MPI
  MPI_Finalize();

  return 0;
}
