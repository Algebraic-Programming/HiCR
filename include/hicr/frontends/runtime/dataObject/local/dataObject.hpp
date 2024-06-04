#pragma once

#include <cstdint>
#include <hicr/core/L0/instance.hpp>

namespace HiCR
{

namespace runtime
{

/**
 * Prototype implementation of the data object class for the host (single instance) runtime mode
 */
class DataObject final
{
  public:

  /**
   * Type definition for a data object id
   */
  typedef uint32_t dataObjectId_t;

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
    // Here an exception is must be produced since the application is exposing this data object to other non-existent instances, leading to a potential deadlock
    // HICR_THROW_LOGIC("Attempting to publish a data object when using the host (single instace) runtime mode.");
  }

  /**
   * Tries to release a previously published data object any instance that wants to take it
   *
   * @return True, if the data object was successfully release (copied to another instance), or was already released; false, if nobody claimed the data object
   */
  __INLINE__ bool tryRelease()
  {
    // Here an exception is must be produced since the application is releasing this data object to other non-existent instances, leading to a potential deadlock
    // HICR_THROW_LOGIC("Attempting to release a data object when using the host (single instace) runtime mode.");

    return false;
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
    // An exception is produced here, since as there is no other instance active, this object will never be retrieved
    HICR_THROW_LOGIC("Attempting to get a data object when using the host (single instance) runtime mode.");

    // Creating data object
    return std::make_shared<DataObject>(nullptr, 0, dataObjectId, currentInstanceId, seed);
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
};

} // namespace runtime

} // namespace HiCR
