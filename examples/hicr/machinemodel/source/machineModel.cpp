#include <hicr/machineModel.hpp>
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
    }

    // More domain specific example: Querying the CPUs and caches topology in detail
    printf("\nDetails of detected CPUs:\n");
    for (auto& d : devices)
    {
        if (d->getType() == "host")
        {
            // Probably redundant cast, since we just need CPU methods and we cast to CPU below...
            HiCR::machineModel::HostDevice *dev = dynamic_cast<HiCR::machineModel::HostDevice *>(d);
            if (dev == nullptr)
                HICR_THROW_FATAL("Error in cast to device");

            auto CPUs = (dev->getComputeResources());
            for (auto c : CPUs)
            {
                HiCR::machineModel::CPU *cpu = dynamic_cast<HiCR::machineModel::CPU *>(c);
                if (cpu == nullptr)
                    HICR_THROW_FATAL("Error in cast to CPU");
                printf(" Core %lu:\n", cpu->getId());
                printf("    Core Siblings ID list:");
                for (unsigned sb : cpu->getSiblings())
                    printf(" %u", sb);
                printf("\n");
                // print the ID of the hardware core (in non-SMT systems that should be equivalent to the cpu ID)
                printf("    System ID: %u\n", cpu->getSystemId());
                printf("    Caches:\n");
                // L1i
                auto cache = cpu->getCache(HiCR::machineModel::Cache::L1i);
                {
                    printf("     L1 instruction:\n");
                    printf("       Size: %lu KB, Line Size: %lu B\n",
                                   cache.getCacheSize() / 1024, cache.getLineSize());
                    if (cache.isShared())
                    {
                        printf("       Shared with CPUs:");
                        for (auto id : cache.getAssociatedComputeUnit())
                            printf(" %lu", id);
                    }
                    else
                        printf("       Private among core siblings");

                    printf("\n");
                }
                // L1d
                cache = cpu->getCache(HiCR::machineModel::Cache::L1d);
                {
                    printf("     L1 data:\n");
                    printf("       Size: %lu KB, Line Size: %lu B\n",
                                   cache.getCacheSize() / 1024, cache.getLineSize());
                    if (cache.isShared())
                    {
                        printf("       Shared with CPUs:");
                        for (auto id : cache.getAssociatedComputeUnit())
                            printf(" %lu", id);
                    }
                    else
                        printf("       Private among core siblings");

                    printf("\n");
                }
                // L2
                cache = cpu->getCache(HiCR::machineModel::Cache::L2);
                {
                    printf("     L2 (unified):\n");
                    printf("       Size: %lu KB, Line Size: %lu B\n",
                                   cache.getCacheSize() / 1024, cache.getLineSize());
                    if (cache.isShared())
                    {
                        printf("       Shared with CPUs:");
                        for (auto id : cache.getAssociatedComputeUnit())
                            printf(" %lu", id);
                    }
                    else
                        printf("       Private among core siblings");

                    printf("\n");
                }
                // L3
                cache = cpu->getCache(HiCR::machineModel::Cache::L3);
                {
                    printf("     L3:\n");
                    printf("       Size: %lu KB, Line Size: %lu B\n",
                                   cache.getCacheSize() / 1024, cache.getLineSize());
                    if (cache.isShared())
                    {
                        printf("       Shared with CPUs:");
                        for (auto id : cache.getAssociatedComputeUnit())
                            printf(" %lu", id);
                    }
                    else
                        printf("       Private among core siblings");

                    printf("\n");
                }
            }

        }
    }

}

