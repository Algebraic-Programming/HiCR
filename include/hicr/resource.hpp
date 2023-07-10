#pragma once

#include <list>
#include <hicr/common.hpp>

namespace HiCR
{

class Resource
{
 public:

 Resource(const resourceId_t id) : _id(id) {};
 ~Resource() = default;

 inline resourceId_t getId() { return _id; }

 protected:

 resourceId_t _id;

};

typedef std::vector<Resource*> resourceList_t;

} // namespace HiCR
