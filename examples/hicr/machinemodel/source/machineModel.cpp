#include <hicr/machineModel.hpp>

#include <cpuDetails.hpp>

int main()
{
  // Instantiate and initialize the machine model.
  // This constructor can be parametrized to white/blacklist the discovery of certain device types
  HiCR::MachineModel m;

  // Detected local devices
  auto devices = m.queryDevices();

  // Iterate over detected devices and print resource information
  for (auto& d : devices)
  {
    // Getting device type (string)
    auto type = d->getType();

    // Getting memory spaces in the device
    auto memSpaces = d->getMemorySpaces();

    // Getting compute resources
    auto computeResources = d->getComputeResources();

    // Printing information
    printf("Detected device: '%s'\n", type.c_str());

    printf(" + Memory Spaces:\n");
    for (auto m : memSpaces)
      printf("    + (%lu) '%s' %fGb\n", m->getId(), m->getType().c_str(), m->getSize() / (1024.f * 1024.f * 1024.f));

    printf(" + Compute Resources:\n");
    for (auto r : computeResources)
      printf("    + (%lu) '%s' \n", r->getId(), r->getType().c_str());

    // Print more details about CPUs
    if (d->getType() == "host")
    {
      // Probably redundant cast, since we just need CPU methods and we cast to CPU below...
      HiCR::machineModel::HostDevice *dev = dynamic_cast<HiCR::machineModel::HostDevice *>(d);
      if (dev == nullptr)
        HICR_THROW_FATAL("Error in cast to device");

      printCpuDetails(dev);
    }

  }

  return 0;
}

