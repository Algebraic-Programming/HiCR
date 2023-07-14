#pragma once

#include <set>

#include <hicr/common.hpp>
#include <hicr/dispatcher.hpp>

namespace HiCR
{

// Definition for resource unique identifiers
typedef uint64_t resourceId_t;

class Resource
{
 public:

 Resource(const resourceId_t id) : _id(id) {};
 virtual ~Resource() = default;

 virtual void initialize() = 0;
 virtual void finalize() = 0;
 virtual void await() = 0;

 inline resourceId_t getId() { return _id; }
 void subscribe(Dispatcher* dispatcher) { _dispatchers.insert(dispatcher); }

 protected:

 // Unique local identifier for the resource
 resourceId_t _id;

 // Dispatchers that this resource is subscribed to
 std::set<Dispatcher*> _dispatchers;
};

typedef std::vector<Resource*> resourceList_t;
} // namespace HiCR
