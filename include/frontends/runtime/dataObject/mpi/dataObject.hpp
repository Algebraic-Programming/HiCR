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
 
   DataObject(void* buffer, const size_t size, const dataObjectId_t id) :
    _buffer(buffer),
    _size(size),
    _id(id)
    {};
   ~DataObject() = default;

   __USED__ inline void publish()
   {
    // Publishing an asynchronous recieve for whoever needs this data
    MPI_Irecv(nullptr, 0, MPI_BYTE, MPI_ANY_SOURCE, _HICR_RUNTIME_DATA_OBJECT_BASE_TAG, MPI_COMM_WORLD, &publishRequest);
   }

   __USED__ inline bool test()
   {
    // If transfered already, return true
    if (_transfered == true) return true;

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
   
    // Changing state to transfered
    _transfered = true;

    // Notifying that the data object has been transferred
    return true;
   }

   dataObjectId_t getId() const { return _id; }

   __USED__ inline static std::shared_ptr<DataObject> getDataObject(DataObject::dataObjectId_t dataObjectId, HiCR::L0::Instance::instanceId_t instanceId)
   {
    // Sending request
    MPI_Send(nullptr, 0, MPI_BYTE, instanceId, _HICR_RUNTIME_DATA_OBJECT_BASE_TAG, MPI_COMM_WORLD);

    // Buffer to store the size
    size_t size = 0;

    // Getting return value size
    MPI_Recv(&size, 1, MPI_UNSIGNED_LONG, instanceId, _HICR_RUNTIME_DATA_OBJECT_RETURN_SIZE_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    // Allocating memory slot to store the return value
    auto buffer = malloc(size);

    // Getting data directly
    MPI_Recv(buffer, size, MPI_BYTE, instanceId, _HICR_RUNTIME_DATA_OBJECT_RETURN_DATA_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    // Creating data object
    return std::make_shared<DataObject>(buffer, size, dataObjectId);
   }

   __USED__ inline void* getData() const { return _buffer; }
   __USED__ inline size_t getSize() const { return _size; }

   __USED__ inline void destroyBuffer()
   {
     free(_buffer);
   }

   private:

   bool _transfered = false;
   void* const _buffer;
   const size_t _size;
   const dataObjectId_t _id;
   MPI_Request publishRequest;

};

} // namespace runtime

} // namespace HiCR
