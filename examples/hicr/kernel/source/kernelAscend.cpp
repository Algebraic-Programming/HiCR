#include <stdio.h>
#include <hicr/backends/ascend/ascend.hpp>
#include <hicr/backends/ascend/device.hpp>
#include <hicr/backends/ascend/kernel.hpp>
#include <hicr.hpp>

int main(int argc, char **argv)
{
 // Instantiating Shared Memory backend
 HiCR::backend::ascend::Ascend backend;

 // Creating compute unit
 HiCR::backend::ascend::Kernel kernel("./kernel.o");

 // Querying compute resources
 backend.queryComputeResources();

 // Getting compute resources
 auto computeResources = backend.getComputeResourceList();

 // Creating processing unit from the compute resource
 HiCR::backend::ascend::Device processingUnit(*computeResources.begin());

 // Initializing processing unit
 processingUnit.initialize();

 // Running compute unit with the processing unit
 processingUnit.start(&kernel);

 return 0;
}

