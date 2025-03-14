#include <CL/opencl.hpp>
#include <hicr/backends/opencl/L1/topologyManager.hpp>
#include "include/topology.hpp"

int main(int argc, char **argv)
{
  // Initializing ascend topology manager
  HiCR::backend::opencl::L1::TopologyManager tm;

  // Calling common function for printing the topology
  topologyFc(tm);

  return 0;
}
