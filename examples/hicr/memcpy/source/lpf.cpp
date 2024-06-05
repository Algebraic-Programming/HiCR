#include <iostream>
#include <lpf/core.h>
#include <lpf/mpi.h>
#include <mpi.h>

#include <hicr/backends/lpf/L1/memoryManager.hpp>
#include <hicr/backends/lpf/L1/communicationManager.hpp>
#include <hicr/backends/host/hwloc/L1/topologyManager.hpp>

#define BUFFER_SIZE 8
#define SENDER_PROCESS 0
#define RECEIVER_PROCESS 1
#define TAG 0
#define DST_OFFSET 0
#define SRC_OFFSET 0
#define CHANNEL_TAG 0

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

void spmd(lpf_t lpf, lpf_pid_t pid, lpf_pid_t nprocs, lpf_args_t args)
{
  // Initializing LPF
  CHECK(lpf_resize_message_queue(lpf, DEFAULT_MSGSLOTS));
  CHECK(lpf_resize_memory_register(lpf, DEFAULT_MEMSLOTS));
  CHECK(lpf_sync(lpf, LPF_SYNC_DEFAULT));

  // Creating HWloc topology object
  hwloc_topology_t topology;

  // Reserving memory for hwloc
  hwloc_topology_init(&topology);

  // Initializing host (CPU) topology manager
  HiCR::backend::host::hwloc::L1::TopologyManager dm(&topology);

  // Asking backend to check the available devices
  const auto t = dm.queryTopology();

  // Getting first device found
  auto d = *t.getDevices().begin();

  // Obtaining memory spaces
  auto memSpaces = d->getMemorySpaceList();

  (void)args; // ignore args parameter passed by lpf_exec
  HiCR::backend::lpf::L1::MemoryManager        m(lpf);
  HiCR::backend::lpf::L1::CommunicationManager c(nprocs, pid, lpf);

  // Getting current process id
  size_t myProcess = pid;

  // Creating local buffer
  auto firstMemSpace = *memSpaces.begin();
  auto localSlot     = m.allocateLocalMemorySlot(firstMemSpace, BUFFER_SIZE);

  // Performing all pending local to global memory slot promotions now
  c.exchangeGlobalMemorySlots(CHANNEL_TAG, {{myProcess, localSlot}});

  // Synchronizing so that all actors have finished registering their global memory slots
  c.fence(CHANNEL_TAG);

  // Getting promoted slot at receiver end
  auto receiverSlot = c.getGlobalMemorySlot(CHANNEL_TAG, RECEIVER_PROCESS);

  if (myProcess == SENDER_PROCESS)
  {
    char *buffer2 = new char[BUFFER_SIZE];
    sprintf(static_cast<char *>(buffer2), "Hi!\n");
    auto srcSlot = m.registerLocalMemorySlot(firstMemSpace, buffer2, BUFFER_SIZE);
    c.memcpy(receiverSlot, DST_OFFSET, srcSlot, SRC_OFFSET, BUFFER_SIZE);
    c.fence(CHANNEL_TAG);
  }

  if (myProcess == RECEIVER_PROCESS)
  {
    c.queryMemorySlotUpdates(localSlot);
    auto recvMsgs = localSlot->getMessagesRecv();
    std::cout << "Received messages (before fence) = " << recvMsgs << std::endl;

    c.fence(CHANNEL_TAG);
    std::cout << "Received buffer = " << static_cast<char *>(localSlot->getPointer()) << std::endl;

    c.queryMemorySlotUpdates(localSlot);
    recvMsgs = localSlot->getMessagesRecv();
    std::cout << "Received messages (after fence) = " << recvMsgs << std::endl;
  }

  // De-registering global slots (collective call)
  c.deregisterGlobalMemorySlot(receiverSlot);

  m.freeLocalMemorySlot(localSlot);
}

int main(int argc, char **argv)
{
  MPI_Init(&argc, &argv);
  lpf_init_t init;
  lpf_args_t args;

  CHECK(lpf_mpi_initialize_with_mpicomm(MPI_COMM_WORLD, &init));
  CHECK(lpf_hook(init, &spmd, args));
  CHECK(lpf_mpi_finalize(init));
  MPI_Finalize();
  return 0;
}
