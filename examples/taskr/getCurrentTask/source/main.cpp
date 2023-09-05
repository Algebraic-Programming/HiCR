#include <cstdio>
#include <cstring>
#include <taskr.hpp>
#include <hicr/backends/sharedMemory/sharedMemory.hpp>

int main(int argc, char **argv)
{
  // Initializing Pthreads backend to run in parallel
  auto t = new HiCR::backend::sharedMemory::SharedMemory();

  // Initializing taskr
  taskr::initialize(t);

  // Single task printing the pointer to
  taskr::addTask(new taskr::Task(0, []()
    {
     auto hicrTask   = HiCR::getCurrentTask();
     auto hicrWorker = hicrTask->getWorker();
     printf("Current HiCR Task pointer:   0x%lX\n", (uint64_t)hicrTask);
     printf("Current HiCR Worker pointer: 0x%lX\n", (uint64_t)hicrWorker);
    }
  ));

  // Running taskr
  taskr::run();

  // Finalizing taskr
  taskr::finalize();

  return 0;
}
