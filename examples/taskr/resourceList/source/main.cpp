#include <cstdio>
#include <cstring>
#include <chrono>
#include <taskr.hpp>
#include <hicr/backends/sharedMemory/sharedMemory.hpp>
#include <workTask.hpp>

#define WORK_TASK_COUNT 100

int main(int argc, char **argv)
{
  // Initializing Pthreads backend to run in parallel
  auto t = new HiCR::backend::sharedMemory::SharedMemory();

  // Initializing compute resources
  t->queryComputeResources();

  // Initializing taskr
  taskr::initialize(t);

  // Getting the core subset from the argument list (could be from a file too)
  HiCR::Backend::computeResourceList_t coreSubset;
  for (int i = 1; i < argc; i++) coreSubset.insert(std::atoi(argv[i]));

  // Adding multiple compute tasks
  printf("Running %u work tasks with %lu cores...\n", WORK_TASK_COUNT, coreSubset.size());
  for (size_t i = 0; i < WORK_TASK_COUNT; i++) taskr::addTask(new taskr::Task(i, &work));

  // Running taskr only on the core subset
  auto t0 = std::chrono::high_resolution_clock::now();
  taskr::run(coreSubset);
  auto tf = std::chrono::high_resolution_clock::now();

  auto dt = std::chrono::duration_cast<std::chrono::nanoseconds>(tf - t0).count();
  printf("Finished in %.3f seconds.\n", (double)dt * 1.0e-9);

  // Finalizing taskr
  taskr::finalize();

  return 0;
}
