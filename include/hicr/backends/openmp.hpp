// This is a minimal backend for multi-core support based on OpenMP

#pragma once

#include <hicr/backend.hpp>
#include <omp.h>

namespace HiCR {

namespace backends {

class OpenMP : public Backend
{
 private:

 public:

 OpenMP() = default;
 ~OpenMP() = default;

 void queryResources() override
 {
  int threadCount = 0;

  // Getting number of openMP threads
  #pragma omp parallel
   #pragma omp master
    threadCount = omp_get_num_threads();

  // Creating resource list, one per openMP thread
  _resourceList.clear();
  for (int i = 0; i < threadCount; i++)
   _resourceList.push_back(new Resource(i));
 }

};


} // namespace backends

} // namespace HiCR

