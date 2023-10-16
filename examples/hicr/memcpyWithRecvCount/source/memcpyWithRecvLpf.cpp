#include <hicr.hpp>
#include <hicr/backends/lpf/lpf.hpp>

#include <lpf/core.h>

#include <iostream>

#define BUFFER_SIZE 256
#define SENDER_PROCESS 0
#define RECEIVER_PROCESS 1
#define TAG 0
#define DST_OFFSET 0
#define SRC_OFFSET 0
#define CHANNEL_TAG 0

void spmd( lpf_t lpf, lpf_pid_t pid, lpf_pid_t nprocs, lpf_args_t args )
{
  (void) args;  // ignore args parameter passed by lpf_exec
  HiCR::backend::lpf::LpfBackend backend(nprocs, pid, lpf);
  backend.queryResources();

  char * buffer1 = new char[BUFFER_SIZE];

  auto dstSlot = backend.registerLocalMemorySlot(buffer1, BUFFER_SIZE);

  // Registering buffers globally for them to be used by remote actors
  backend.promoteMemorySlotToGlobal(CHANNEL_TAG, SENDER_PROCESS, dstSlot);
  // Performing all pending local to global memory slot promotions now
  backend.exchangeGlobalMemorySlots(CHANNEL_TAG);

  //sleep(15);
  // Synchronizing so that all actors have finished registering their global memory slots
  backend.fence(CHANNEL_TAG);

  int myProcess = backend.getProcessId();
  /**
   * Collective exchange of local slots to be
   * promoted to global. In this case:
   * RECEIVER_PROCESS promotes his slot to a global slot.
   * SENDER_PROCESS does not promote any slots to global --
   * but will allocate a local sender slot.
   */
 // Obtaining the globally exchanged memory slots
 std::map<uint64_t, std::vector<uint64_t>> globalBuffers = backend.getGlobalMemorySlots()[CHANNEL_TAG];

  if (myProcess == SENDER_PROCESS)
  {
      char * buffer2 = new char[BUFFER_SIZE];
      sprintf(static_cast<char *>(buffer2), "Hello, HiCR user!\n");
      auto srcSlot = backend.registerLocalMemorySlot(buffer2, BUFFER_SIZE);
      //sleep(15);
      backend.memcpy(globalBuffers[CHANNEL_TAG][1], DST_OFFSET, srcSlot, SRC_OFFSET, BUFFER_SIZE);
      backend.fence(CHANNEL_TAG);
  }

  if (myProcess == RECEIVER_PROCESS)  {
      auto recvMsgs = backend.getMemorySlotReceivedMessages(globalBuffers[CHANNEL_TAG][1]);
      std::cout << "Received messages (before fence) = " << recvMsgs << std::endl;
      backend.fence(CHANNEL_TAG);
      std::cout << "Received buffer = " << static_cast<const char*>(buffer1);
      recvMsgs = backend.getMemorySlotReceivedMessages(globalBuffers[CHANNEL_TAG][1]);
      std::cout << "Received messages = " << recvMsgs << std::endl;
  }
  
}

int main(int argc, char **argv)
{

  lpf_args_t args;
  memset(&args, 0, sizeof(lpf_args_t));
  args.input = argv;
  args.input_size = argc;
  lpf_err_t rc = lpf_exec( LPF_ROOT, LPF_MAX_P, &spmd, LPF_NO_ARGS);
  EXPECT_EQ("%d", LPF_SUCCESS, rc);

  return 0;
}
