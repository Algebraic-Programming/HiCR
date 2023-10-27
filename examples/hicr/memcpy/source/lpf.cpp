#include <hicr.hpp>
#include <hicr/backends/lpf/memoryManager.hpp>
#include <hicr/memorySlot.hpp>

#include <lpf/core.h>
#include <lpf/mpi.h>
#include <mpi.h>

#include <iostream>

#define BUFFER_SIZE 256
#define SENDER_PROCESS 0
#define RECEIVER_PROCESS 1
#define TAG 0
#define DST_OFFSET 0
#define SRC_OFFSET 0
#define CHANNEL_TAG 0

// flag needed when using MPI to launch
const int LPF_MPI_AUTO_INITIALIZE = 0;

void spmd( lpf_t lpf, lpf_pid_t pid, lpf_pid_t nprocs, lpf_args_t args )
{
  (void) args;  // ignore args parameter passed by lpf_exec
  HiCR::backend::lpf::MemoryManager m(nprocs, pid, lpf);
  size_t myProcess = pid;

  char * buffer1 = new char[BUFFER_SIZE];

  auto dstSlot = m.registerLocalMemorySlot(buffer1, BUFFER_SIZE);
  std::vector<std::pair<size_t, HiCR::MemorySlot *> > promoted;
  promoted.push_back(std::make_pair(myProcess,dstSlot));

  // Performing all pending local to global memory slot promotions now
  m.exchangeGlobalMemorySlots(CHANNEL_TAG, promoted);

  // Synchronizing so that all actors have finished registering their global memory slots
  m.fence(CHANNEL_TAG);

  HiCR::MemorySlot * myPromotedSlot;

  if (myProcess == SENDER_PROCESS)
  {
      char * buffer2 = new char[BUFFER_SIZE];
      sprintf(static_cast<char *>(buffer2), "Hello, HiCR user!\n");
      auto srcSlot = m.registerLocalMemorySlot(buffer2, BUFFER_SIZE);
      //sleep(15);
      myPromotedSlot = m.getGlobalMemorySlot(CHANNEL_TAG, RECEIVER_PROCESS);
      m.memcpy(myPromotedSlot, DST_OFFSET, srcSlot, SRC_OFFSET, BUFFER_SIZE);
      m.fence(CHANNEL_TAG);
  }

  if (myProcess == RECEIVER_PROCESS)  {
      myPromotedSlot = m.getGlobalMemorySlot(CHANNEL_TAG, RECEIVER_PROCESS);
      m.queryMemorySlotUpdates(myPromotedSlot);    
      auto recvMsgs = myPromotedSlot->getMessagesRecv();
      std::cout << "Received messages (before fence) = " << recvMsgs << std::endl;
      m.fence(CHANNEL_TAG);
      std::cout << "Received buffer = " << static_cast<const char*>(myPromotedSlot->getPointer());
      m.queryMemorySlotUpdates(myPromotedSlot);    
      recvMsgs = myPromotedSlot->getMessagesRecv();
      std::cout << "Received messages (after fence) = " << recvMsgs << std::endl;
  }
  
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
