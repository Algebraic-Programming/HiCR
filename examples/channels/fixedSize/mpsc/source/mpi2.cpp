#include <frontends/channel/fixedSize/mpsc/consumer.hpp>
#include <frontends/channel/fixedSize/mpsc/producer.hpp>
#include <backends/mpi/L1/memoryManager.hpp>
#include <backends/mpi/L1/communicationManager.hpp>
#include <backends/host/hwloc/L1/topologyManager.hpp>
#include <mpi.h>

#include <iostream>

#define TOKEN_TAG 1
#define BUFFER_TAG 2
#define ELEMENT_TYPE unsigned int

int main(int argc, char **argv)
{
  // Initializing MPI
  MPI_Init(&argc, &argv);

  // Getting MPI values
  int rankCount = 0;
  int rankId = 0;
  MPI_Comm_rank(MPI_COMM_WORLD, &rankId);
  MPI_Comm_size(MPI_COMM_WORLD, &rankCount);

  // Sanity Check
  if (rankCount < 2)
  {
    if (rankId == 0) fprintf(stderr, "Launch error: MPI process count must be at least 2\n");
    return MPI_Finalize();
  }

  // Reading argument
  auto channelCapacity = 10;


  // Instantiating backend
  HiCR::backend::mpi::L1::MemoryManager m;
  HiCR::backend::mpi::L1::CommunicationManager c(MPI_COMM_WORLD);

  // Creating HWloc topology object
  hwloc_topology_t topology;

  // Reserving memory for hwloc
  hwloc_topology_init(&topology);

  // Initializing HWLoc-based host (CPU) topology manager
  HiCR::backend::host::hwloc::L1::TopologyManager tm(&topology);

  // Asking backend to check the available devices
  const auto t = tm.queryTopology();

  // Getting first device found
  auto d = *t.getDevices().begin();

  // Obtaining memory spaces
  auto memSpaces = d->getMemorySpaceList();

  // Getting reference to the first memory space detected
  auto firstMemorySpace = *memSpaces.begin();

  // Getting required buffer sizes
  auto tokenBufferSize = HiCR::channel::fixedSize::Base::getTokenBufferSize(sizeof(ELEMENT_TYPE), channelCapacity);

  auto coordinationBufferSize = HiCR::channel::fixedSize::Base::getCoordinationBufferSize();
  // Registering token buffer as a local memory slot
  auto coordinationBuffer = m.allocateLocalMemorySlot(firstMemorySpace, coordinationBufferSize);

  // Initializing coordination buffer (sets to zero the counters)
  HiCR::channel::fixedSize::Base::initializeCoordinationBuffer(coordinationBuffer);
  std::shared_ptr<HiCR::L0::LocalMemorySlot> tokenBufferSlot;

  std::vector<std::shared_ptr<HiCR::channel::fixedSize::MPSC::Producer> > producers(rankCount);
  std::shared_ptr<HiCR::channel::fixedSize::MPSC::Consumer> consumer;
  for (int i=0; i<rankCount; i++) {
	  // 1 consumer
	  if (rankId == i) {
		  tokenBufferSlot = m.allocateLocalMemorySlot(firstMemorySpace, tokenBufferSize);
		  c.exchangeGlobalMemorySlots(TOKEN_TAG, {{i, tokenBufferSlot}});
		  c.fence(TOKEN_TAG);
		  c.exchangeGlobalMemorySlots(BUFFER_TAG, {{i, coordinationBuffer}});
		  c.fence(BUFFER_TAG);
		  auto globalTokenBufferSlot = c.getGlobalMemorySlot(TOKEN_TAG, i);
		  auto consumerCoordinationBuffer = c.getGlobalMemorySlot(BUFFER_TAG, i);
		  consumer = std::shared_ptr<HiCR::channel::fixedSize::MPSC::Consumer>(new HiCR::channel::fixedSize::MPSC::Consumer(c, globalTokenBufferSlot, coordinationBuffer, consumerCoordinationBuffer, sizeof(ELEMENT_TYPE), channelCapacity));
	  }
	  // 1-to-many producers
	  else {
		  c.exchangeGlobalMemorySlots(TOKEN_TAG, {});
		  c.fence(TOKEN_TAG);
		  c.exchangeGlobalMemorySlots(BUFFER_TAG, {});
		  c.fence(BUFFER_TAG);
		  auto globalTokenBufferSlot = c.getGlobalMemorySlot(TOKEN_TAG, i);
		  auto consumerCoordinationBuffer = c.getGlobalMemorySlot(BUFFER_TAG, i);
		  producers[i] = std::shared_ptr<HiCR::channel::fixedSize::MPSC::Producer>(new HiCR::channel::fixedSize::MPSC::Producer(c, globalTokenBufferSlot, coordinationBuffer, consumerCoordinationBuffer, sizeof(ELEMENT_TYPE), channelCapacity));
	  }
  }

  // You push to the other ranks
  for (int i=0; i<rankCount; i++) {
    if (rankId != i) {
      // Everyone should produce a message
      auto senderBuffer = m.allocateLocalMemorySlot(firstMemorySpace, sizeof(ELEMENT_TYPE));
      ELEMENT_TYPE * senderBufferPtr = static_cast<ELEMENT_TYPE *>(senderBuffer->getPointer());
      senderBufferPtr[0] = rankId;
      while (producers[i]->push(senderBuffer) != true);    
      std::cout << "Rank " << rankId << " pushed an element to producer " << i << " with value " << senderBufferPtr[0] << std::endl;
    }
  }

  // I expect to receive an element from all producers (except me)
  if (rankId == 0) {
	  for (int i=0; i<rankCount-1; i++) {
		  auto pos = consumer->peek();
		  while (pos < 0) pos = consumer->peek();

		  auto tokenBuffer = static_cast<ELEMENT_TYPE *>(tokenBufferSlot->getPointer());
		  std::cout << "Rank " << rankId << " popped element with value = " << tokenBuffer[pos] << std::endl;
		  consumer->pop();
	  }

  }

  // Finalizing MPI
  MPI_Finalize();

  return 0;
}
