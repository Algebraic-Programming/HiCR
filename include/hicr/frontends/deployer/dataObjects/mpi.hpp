#pragma once

#include <cstdint>
#include <memory>
#include <mpi.h>
#include <hicr/core/L0/instance.hpp>
#include "../dataObject.hpp"

enum
{
  _HICR_DEPLOYER_DATA_OBJECT_BASE_TAG = 0x00010000
};
#define _HICR_DEPLOYER_DATA_OBJECT_RETURN_SIZE_TAG _HICR_DEPLOYER_DATA_OBJECT_BASE_TAG + 1
#define _HICR_DEPLOYER_DATA_OBJECT_RETURN_DATA_TAG _HICR_DEPLOYER_DATA_OBJECT_BASE_TAG + 2

namespace HiCR::deployer::dataObject
{

/**
 * Prototype implementation of the data object class for MPI
 */
class MPI final : public HiCR::deployer::DataObject
{
  public:

  /**
   * Type definition for a data object id
   */
  using dataObjectId_t = uint32_t;

  /**
   * Mask used to convert the data object ID to the precision required by the MPI tags (15 bits guaranteed by the specification).
   *
   * Notes: using only 15 bits of the data object id increases the risk of collisions
   *
   * See: https://www.intel.com/content/www/us/en/developer/articles/technical/large-mpi-tags-with-the-intel-mpi.html#:~:text=For%20the%20InfiniBand*%20support%20via,be%20queried%20in%20the%20application
   */
  static constexpr dataObjectId_t mpiTagMask = 32767;

  /**
   * Standard data object constructor
   *
   * @param[in] buffer The internal buffer to use for the data object
   * @param[in] size The size of the internal buffer
   * @param[in] id Identifier for the new data object
   * @param[in] instanceId Identifier of the instance owning the data object
   * @param[in] seed random application seed  
   */
  MPI(void *const buffer, const size_t size, const dataObjectId_t id, const HiCR::L0::Instance::instanceId_t instanceId, const HiCR::L0::Instance::instanceId_t seed)
    : DataObject(buffer, size, id, instanceId, seed)
  {}

  ~MPI() = default;

  /**
   * Exposes a data object to be obtained (stealed) by another instance
   */
  __INLINE__ void publish()
  {
    // Do nothing if the data object has already been published
    if (_isPublished == true) { return; }
    // Pick the first 15 bits of the id and use it as MPI Tag
    const int dataObjectIdTag = (int)getId() & (int)mpiTagMask;

    publishRequest = MPI_REQUEST_NULL;

    // Publishing an asynchronous recieve for whoever needs this data
    MPI_Irecv(nullptr, 0, MPI_BYTE, MPI_ANY_SOURCE, dataObjectIdTag, MPI_COMM_WORLD, &publishRequest);
    _isPublished = true;
  }

  /**
   * Mark the object available for publication again
   */
  __INLINE__ void unpublish() { _isPublished = false; }

  /**
   * Tries to release a previously published data object any instance that wants to take it
   *
   * @return True, if the data object was successfully release (copied to another instance), or was already released; false, if nobody claimed the data object
   */
  __INLINE__ bool tryRelease()
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

    // Getting RPC execution unit index
    MPI_Ssend(getData(), (int)getSize(), MPI_BYTE, requester, _HICR_DEPLOYER_DATA_OBJECT_RETURN_DATA_TAG, MPI_COMM_WORLD);

    // Mark the object as not published
    _isPublished = false;

    // Notifying that the data object has been transferred
    return true;
  }

  /**
   * Obtains a data object from a remote instance, based on its id
   *
   * This function will stall until and unless the specified remote instance published the given data object
   *
   * @param[in] currentInstanceId Id of the instance that will own the data object
   * @param[in] seed unique random seed 
   */
  __INLINE__ void get(HiCR::L0::Instance::instanceId_t currentInstanceId, HiCR::L0::Instance::instanceId_t seed) override
  {
    // Pick the first 15 bits of the id and use it as MPI Tag
    const int dataObjectIdTag = (int)getId() & (int)mpiTagMask;

    // Get the corresponding MPI rank where this data object is located
    int sourceRank = (int)getInstanceId();

    // Sending request
    MPI_Send(nullptr, 0, MPI_BYTE, sourceRank, dataObjectIdTag, MPI_COMM_WORLD);

    // Getting data directly
    MPI_Recv(getData(), (int)getSize(), MPI_BYTE, sourceRank, _HICR_DEPLOYER_DATA_OBJECT_RETURN_DATA_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  }

  private:

  /**
   * MPI-specific implementation Request object.
   */
  MPI_Request publishRequest = nullptr;

  /**
   * Indicates whether the object has been published or not
   */
  bool _isPublished = false;
};

} // namespace HiCR::deployer::dataObject
