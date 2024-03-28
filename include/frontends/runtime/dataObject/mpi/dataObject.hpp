#pragma once

#include <cstdint>
#include <hicr/L0/instance.hpp>
#include <mpi.h>

#define _HICR_RUNTIME_DATA_OBJECT_BASE_TAG 0x00010000
#define _HICR_RUNTIME_DATA_OBJECT_RETURN_SIZE_TAG _HICR_RUNTIME_DATA_OBJECT_BASE_TAG + 1
#define _HICR_RUNTIME_DATA_OBJECT_RETURN_DATA_TAG _HICR_RUNTIME_DATA_OBJECT_BASE_TAG + 2

namespace HiCR
{

namespace runtime
{

/**
 * Prototype implementation of the data object class for MPI
 */
class DataObject final
{
  public:

  /**
   * Type definition for a data object id
   */
  typedef uint32_t dataObjectId_t;

  /**
   * Mask used to convert the data object ID to the precision required by the MPI tags (15 bits guaranteed by the specification).
   *
   * Notes: using only 15 bits of the data object id increases the risk of collisions
   *
   * See: https://www.intel.com/content/www/us/en/developer/articles/technical/large-mpi-tags-with-the-intel-mpi.html#:~:text=For%20the%20InfiniBand*%20support%20via,be%20queried%20in%20the%20application
   */
  static const dataObjectId_t mpiTagMask = 32767;

  /**
   * Standard data object constructor
   *
   * @param[in] buffer The internal buffer to use for the data object
   * @param[in] size The size of the internal buffer
   * @param[in] id Identifier for the new data object
   * @param[in] instanceId Identifier of the instance owning the data object
   * @param[in] seed random application seed  
   */
  DataObject(void *buffer, const size_t size, const dataObjectId_t id, const HiCR::L0::Instance::instanceId_t instanceId, const HiCR::L0::Instance::instanceId_t seed)
    : _buffer(buffer),
      _size(size),
      _id(id){};
  ~DataObject() = default;

  /**
   * Exposes a data object to be obtained (stealed) by another instance
   */
  __INLINE__ void publish()
  {
    // Pick the first 15 bits of the id and use it as MPI Tag
    const int dataObjectIdTag = _id & mpiTagMask;

    // Publishing an asynchronous recieve for whoever needs this data
    MPI_Irecv(nullptr, 0, MPI_BYTE, MPI_ANY_SOURCE, dataObjectIdTag, MPI_COMM_WORLD, &publishRequest);
  }

  /**
   * Tries to release a previously published data object any instance that wants to take it
   *
   * @return True, if the data object was successfully release (copied to another instance), or was already released; false, if nobody claimed the data object
   */
  __INLINE__ bool release()
  {
    // If transfered already, return true
    if (_isReleased == true) return true;

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
    _isReleased = true;

    // Notifying that the data object has been transferred
    return true;
  }

  /**
   * Gets the data object id
   *
   * @return The data object id
   */
  dataObjectId_t getId() const { return _id; }

  /**
   * Obtains a data object from a remote instance, based on its id
   *
   * This function will stall until and unless the specified remote instance published the given data object
   *
   * @param[in] dataObjectId The data object id to take from a remote instance
   * @param[in] remoteInstanceId Id of the remote instance to take the published data object from
   * @param[in] currentInstanceId Id of the instance that will own the data object
   * @param[in] seed unique random seed 
   * @return A shared pointer to the data object obtained from the remote instance
   */
  __INLINE__ static std::shared_ptr<DataObject> getDataObject(DataObject::dataObjectId_t       dataObjectId,
                                                                   HiCR::L0::Instance::instanceId_t remoteInstanceId,
                                                                   HiCR::L0::Instance::instanceId_t currentInstanceId,
                                                                   HiCR::L0::Instance::instanceId_t seed)
  {
    // Pick the first 15 bits of the id and use it as MPI Tag
    const int dataObjectIdTag = dataObjectId & mpiTagMask;

    // Sending request
    MPI_Send(nullptr, 0, MPI_BYTE, remoteInstanceId, dataObjectIdTag, MPI_COMM_WORLD);

    // Buffer to store the size
    size_t size = 0;

    // Getting return value size
    MPI_Recv(&size, 1, MPI_UNSIGNED_LONG, remoteInstanceId, _HICR_RUNTIME_DATA_OBJECT_RETURN_SIZE_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    // Allocating memory slot to store the return value
    auto buffer = malloc(size);

    // Getting data directly
    MPI_Recv(buffer, size, MPI_BYTE, remoteInstanceId, _HICR_RUNTIME_DATA_OBJECT_RETURN_DATA_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    // Creating data object
    return std::make_shared<DataObject>(buffer, size, dataObjectId, currentInstanceId, seed);
  }

  /**
   * Gets access to the internal data object buffer
   *
   * @return A pointer to the internal data object buffer
   */
  __INLINE__ void *getData() const { return _buffer; }

  /**
   * Gets the size of the data object's internal data buffer
   *
   * @return The size of the data object's internal data buffer
   */
  __INLINE__ size_t getSize() const { return _size; }

  /**
   * Frees up the internal buffer of the data object.
   *
   * The same semantics of a normal free() function applies. Double frees must be avoided.
   */
  __INLINE__ void destroyBuffer() { free(_buffer); }

  private:

  /**
   * Stores whether the data object has been already released to another instance and no longer owned by this one
   */
  bool _isReleased = false;

  /**
   * The data object's internal data buffer
   */
  void *const _buffer;

  /**
   * The data object's internal data size
   */
  const size_t _size;

  /**
   * The data object's identifier
   */
  const dataObjectId_t _id;

  /**
   * MPI-specific implementation Request object.
   */
  MPI_Request publishRequest;
};

} // namespace runtime

} // namespace HiCR
