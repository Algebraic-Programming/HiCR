// This is a minimal backend for multi-core support based on OpenMP

#pragma once

#include <hicr/resource.hpp>

namespace HiCR {

namespace backends {

namespace openMP {


class Thread : public Resource
{
 private:

 public:

 Thread(const resourceId_t id) : Resource(id) {};
 ~Thread() = default;

};

} // namespace openMP

} // namespace backends

} // namespace HiCR

