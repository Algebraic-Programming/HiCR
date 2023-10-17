#include <hwloc.h>
#include <hicr/backends/sequential/computeManager.hpp>
#include "abcTasks.hpp"

int main(int argc, char **argv)
{
  // Initializing Pthreads backend to run in parallel
  HiCR::backend::sequential::ComputeManager computeManager;

  // Running ABCtasks example
  abcTasks(&computeManager);

  return 0;
}
