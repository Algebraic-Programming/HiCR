
// More domain specific example: Querying the CPUs and caches topology in detail
void printCpuDetails(HiCR::machineModel::HostDevice *dev)
{
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
  /* Alternative approach: set the following vector an iterate over it to get each cache:
  std::vector caches = { HiCR::machineModel::Cache::L1i,
                         HiCR::machineModel::Cache::L1d,
                         HiCR::machineModel::Cache::L2,
                         HiCR::machineModel::Cache::L3 }
  */
  }
}

