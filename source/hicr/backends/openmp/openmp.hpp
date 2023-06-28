// This is a minimal backend for multi-core support based on OpenMP

#pragma once

#include <hicr/backends/backend.hpp>

namespace HiCR {

namespace backends {

class OpenMP : public Backend
{
 private:

 public:

 OpenMP() = default;
 ~OpenMP() = default;
 resourceList_t queryResources() override;

};


} // namespace backends

} // namespace HiCR

