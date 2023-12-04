#include "include/abcTasks.hpp"
#include <hicr/backends/sequential/computeManager.hpp>
#include <hwloc.h>

int main(int argc, char **argv)
{
  // Initializing sequential backend
  HiCR::backend::sequential::ComputeManager computeManager;

  // Running ABCtasks example
  abcTasks(&computeManager);

  return 0;
}
