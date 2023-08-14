#pragma once

#include <hicr/resource.hpp>

namespace HiCR
{

class Runtime;

class Backend
{
  friend class Runtime;

  protected:
  Backend() = default;

  public:
  virtual ~Backend() = default;
  virtual void queryResources() = 0;
  inline resourceList_t &getResourceList() { return _resourceList; }

  protected:
  resourceList_t _resourceList;
};

} // namespace HiCR
