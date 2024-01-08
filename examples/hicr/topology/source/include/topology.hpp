#include <hicr/L1/topologyManager.hpp>

void topologyExchange(HiCR::L1::TopologyManager &t)
{
  // Querying devices visible by the passed topology manager
  t.queryDevices();

  // Serializing the detected topology
  auto topology = t.serialize();
 
  // Now deserializing the detected topology
  t.deserialize(topology);

  // Printing all devices
  printf("Devices: \n");
  for (const auto& d : t.getDevices())
  {
    printf("  + '%s'\n", d->getType().c_str());
    printf("    Compute Resources: %lu %s(s)\n", d->getComputeResourceList().size(), (*d->getComputeResourceList().begin())->getType().c_str());
    // for (const auto& c : d->getComputeResourceList())  printf("    + Compute Resource: '%s'\n", c->getType().c_str());
    for (const auto& m : d->getMemorySpaceList())      printf("    Memory Space:     '%s', %f Gb\n", m->getType().c_str(), (double)m->getSize() / (double)(1024ul*1024ul*1024ul));
  }
}