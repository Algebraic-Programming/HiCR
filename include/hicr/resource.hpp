#pragma once

#include <set>

#include <hicr/common.hpp>
#include <hicr/dispatcher.hpp>

namespace HiCR
{

// Definition for resource unique identifiers
typedef uint64_t resourceId_t;

// Definition for function to run at resource
typedef std::function<void(void)> resourceFc_t;

class Resource
{
 public:

 Resource(const resourceId_t id) : _id(id) {};
 virtual ~Resource() = default;

 virtual void initialize() = 0;
 virtual void run(resourceFc_t fc) = 0;
 virtual void finalize() = 0;
 virtual void await() = 0;

 inline resourceId_t getId() { return _id; }

 protected:

 // Unique local identifier for the resource
 resourceId_t _id;

 // Copy of a function to run
 resourceFc_t _fc;
};

typedef std::vector<Resource*> resourceList_t;
} // namespace HiCR
