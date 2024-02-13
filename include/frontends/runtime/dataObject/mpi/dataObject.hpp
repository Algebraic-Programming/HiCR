#pragma once

#include <cstdint>
#include <mpi.h>

#define _HICR_RUNTIME_DATA_OBJECT_BASE_TAG 0x00010000
#define _HICR_RUNTIME_DATA_OBJECT_RETURN_SIZE_TAG _HICR_RUNTIME_DATA_OBJECT_BASE_TAG + 1
#define _HICR_RUNTIME_DATA_OBJECT_RETURN_DATA_TAG _HICR_RUNTIME_DATA_OBJECT_BASE_TAG + 2

namespace HiCR
{

namespace runtime
{

class DataObject final
{
   public:
   
   typedef uint32_t dataObjectId_t;
 
   DataObject(const void* buffer, const size_t size, const dataObjectId_t id) :
    _buffer(buffer),
    _size(size),
    _id(id)
    {};
   ~DataObject() = default;

   void publish()
   {
    // Publishing an asynchronous recieve for whoever needs this data
    MPI_Irecv(nullptr, 0, MPI_BYTE, MPI_ANY_SOURCE, _HICR_RUNTIME_DATA_OBJECT_BASE_TAG + _id, MPI_COMM_WORLD, &publishRequest);
   }

   bool test()
   {
    // We need to preserve the status to receive more information about the RPC
    MPI_Status status;

    // Flag indicating whether the message has been received
    int flag = 0;

    // Testing whether the publication has been claimed
    MPI_Test(&publishRequest, &flag, &status);

    // If not, return immediately 
    if (flag == 0) return false;

    // Otherwise send message size and contents immediately
    auto requester = status.MPI_SOURCE;

    // Sending message size first
    MPI_Ssend(&_size, 1, MPI_UNSIGNED_LONG, requester, _HICR_RUNTIME_DATA_OBJECT_RETURN_SIZE_TAG, MPI_COMM_WORLD);

    // Getting RPC execution unit index
    MPI_Ssend(_buffer, _size, MPI_BYTE, requester, _HICR_RUNTIME_DATA_OBJECT_RETURN_DATA_TAG, MPI_COMM_WORLD);

    // Notifying that the data object has been transferred
    return true;
   }

   dataObjectId_t getId() const { return _id; }

   private:

   const void* const _buffer;
   const size_t _size;
   const dataObjectId_t _id;
   MPI_Request publishRequest;

};

} // namespace runtime

} // namespace HiCR
