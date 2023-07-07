#pragma once

#include <hicr/resource.hpp>

namespace HiCR {

class Backend
{
 private:

 public:

 Backend() = default;
 virtual ~Backend() = default;
 virtual void queryResources() = 0;
 inline resourceList_t& getResourceList() { return _resourceList; }

 protected:

 resourceList_t _resourceList;
};


} // namespace HiCR
