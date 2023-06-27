// This is a minimal backend for multi-core support based on OpenMP

#pragma once

#include <backends/backend.hpp>

namespace HiCR {

namespace backends {

class OpenMP : public Backend
{
 private:

 public:

 resourceList_t queryResources() override;

};


} // namespace backends

} // namespace HiCR

