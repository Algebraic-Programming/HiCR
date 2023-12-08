#include <hwloc.h>
#include <hicr/backends/sequential/L1/computeManager.hpp>
#include "include/abcTasks.hpp"

int main(int argc, char **argv)
{
  // Initializing sequential backend
  HiCR::backend::sequential::L1::ComputeManager computeManager;

  // Running ABCtasks example
  abcTasks(&computeManager);

  return 0;
}
