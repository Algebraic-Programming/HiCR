#include <stdio.h>
#include <hicr/backends/sequential/sequential.hpp>
#include <hicr/backends/sequential/function.hpp>
#include <hicr.hpp>

int main(int argc, char **argv)
{
 // Instantiating Shared Memory backend
 HiCR::backend::sequential::Sequential backend;

 auto fcLambda = [](void* arg) { printf("Hello, World!\n");};

 // Creating compute unit
 HiCR::backend::sequential::Function fc(fcLambda);

 // Querying compute resources
 backend.queryComputeResources();

 // Getting compute resources
 auto computeResources = backend.getComputeResourceList();

 // Creating processing unit from the compute resource
 HiCR::backend::sequential::Process processingUnit(*computeResources.begin());

 // Initializing processing unit
 processingUnit.initialize();

 // Running compute unit with the processing unit
 processingUnit.start(&fc);

 return 0;
}

