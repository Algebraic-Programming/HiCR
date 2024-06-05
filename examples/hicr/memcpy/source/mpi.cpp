#include <iostream>
#include <mpi.h>
#include <hicr/backends/mpi/L1/memoryManager.hpp>
#include <hicr/backends/mpi/L1/communicationManager.hpp>
#include <hicr/backends/host/hwloc/L1/topologyManager.hpp>

#define BUFFER_SIZE 8
#define SENDER_PROCESS 0
#define RECEIVER_PROCESS 1
#define TAG 0
#define DST_OFFSET 0
#define SRC_OFFSET 0
#define CHANNEL_TAG 0

int main(int argc, char **argv)
{
  MPI_Init(&argc, &argv);
  MPI_Comm comm = MPI_COMM_WORLD;
  int      rank;
  MPI_Comm_rank(comm, &rank);

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

  HiCR::backend::mpi::L1::MemoryManager        m;
  HiCR::backend::mpi::L1::CommunicationManager c(comm);

  // Getting current process id
  size_t myProcess = rank;

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

  MPI_Finalize();
}
