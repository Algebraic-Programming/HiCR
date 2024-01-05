#include <hicr/L1/topologyManager.hpp>

void topologyPrinter(HiCR::L1::TopologyManager &t)
{
  // Querying devices visible by the passed topology manager
  t.queryDevices();

  // Serializing the detected topology
  auto topology = t.serialize();

  // Printing detected topology
  printf("%s\n", topology.dump(2).c_str());
}