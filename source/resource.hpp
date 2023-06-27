#pragma once

#include <list>

namespace HiCR
{

class Resource
{
 private:

 public:

 Resource() = default;
 ~Resource() = default;
 size_t id;

};

typedef std::list<Resource> resourceList_t;

} // namespace HiCR
