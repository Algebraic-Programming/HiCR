#include <hwloc.h>
#include <hicr/backends/sequential/computeManager.hpp>
#include "abcTasks.hpp"

int main(int argc, char **argv)
{
  // Initializing sequential backend
  HiCR::backend::sequential::ComputeManager computeManager;

  // Running ABCtasks example
  abcTasks(&computeManager);

  return 0;
}
