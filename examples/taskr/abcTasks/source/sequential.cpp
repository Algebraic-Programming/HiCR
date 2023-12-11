#include "include/abcTasks.hpp"
#include <backends/sequential/L1/computeManager.hpp>
#include <hwloc.h>

int main(int argc, char **argv)
{
  // Initializing sequential backend
  HiCR::backend::sequential::L1::ComputeManager computeManager;

  // Running ABCtasks example
  abcTasks(&computeManager);

  return 0;
}
