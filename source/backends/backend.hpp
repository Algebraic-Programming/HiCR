#pragma once

#include <hicr/resource.hpp>

namespace HiCR {

class Backend
{
 private:

 public:

 Backend() = default;
 ~Backend() = default;
 virtual resourceList_t queryResources() = 0;

};


} // namespace HiCR
