#include <acl/acl.h>
#include <hicr/backends/ascend/L1/topologyManager.hpp>
#include "include/topology.hpp"

int main(int argc, char **argv)
{
  // Initialize (Ascend's) ACL runtime
  aclError err = aclInit(NULL);
  if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Failed to initialize Ascend Computing Language. Error %d", err);

  // Initializing ascend topology manager
  HiCR::backend::ascend::L1::TopologyManager tm;

  // Calling common function for printing the topology
  topologyFc(tm);

  return 0;
}
