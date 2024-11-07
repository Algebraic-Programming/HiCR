#pragma once

#include <cstdint>
#include <memory>
#include <hicr/core/L0/instance.hpp>
#include <hicr/core/definitions.hpp>
#include <hicr/core/exceptions.hpp>

namespace HiCR::deployer
{

/**
 * Prototype implementation of the data object class for the host (single instance) deployer mode
 */
class DataObject final
{
  public:

  /**
   * Type definition for a data object id
   */
  using dataObjectId_t = uint32_t;

  /**
   * Standard data object constructor
   *
   * @param[in] buffer The internal buffer to use for the data object
   * @param[in] size The size of the internal buffer
   * @param[in] id Identifier for the new data object
   * @param[in] instanceId Identifier of the instance owning the data object
   * @param[in] seed random application seed  
   */
  DataObject(void *const buffer, const std::size_t size, const dataObjectId_t id, const HiCR::L0::Instance::instanceId_t instanceId, const HiCR::L0::Instance::instanceId_t seed)
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
    // HICR_THROW_LOGIC("Attempting to publish a data object when using the host (single instace) deployer mode.");
  }

  /**
   * Mark the data object available for publication
   */
  __INLINE__ void unpublish() {}

  /**
   * Tries to release a previously published data object any instance that wants to take it
   *
   * @return True, if the data object was successfully release (copied to another instance), or was already released; false, if nobody claimed the data object
   */
  __INLINE__ bool tryRelease()
  {
    // Here an exception is must be produced since the application is releasing this data object to other non-existent instances, leading to a potential deadlock
    // HICR_THROW_LOGIC("Attempting to release a data object when using the host (single instace) deployer mode.");

    return false;
  }

  /**
   * Obtains a data object from a remote instance, based on its id
   *
   * This function will stall until and unless the specified remote instance published the given data object
   *
   * @param[inout] dataObject The data object to take from a remote instance
   * @param[in] currentInstanceId Id of the instance that will own the data object
   * @param[in] seed unique random seed 
   */
  __INLINE__ static void getDataObject(HiCR::deployer::DataObject &dataObject, HiCR::L0::Instance::instanceId_t currentInstanceId, HiCR::L0::Instance::instanceId_t seed)
  {
    // An exception is produced here, since as there is no other instance active, this object will never be retrieved
    HICR_THROW_LOGIC("Attempting to get a data object when using the host (single instance) deployer mode.");
  }

  /**
   * Gets the data object id
   *
   * @return The data object id
   */
  [[nodiscard]] dataObjectId_t getId() const { return _id; }

  /**
   * Set the data object id
   *
   * @param[in] id The data object id
   */
  void setId(const dataObjectId_t id) { _id = id; }

  /**
   * Gets the instance id
   *
   * @return The data object instance id
   */
  HiCR::L0::Instance::instanceId_t getInstanceId()
  {
    // Default to 0 since its local
    return 0;
  }

  /**
   * Gets access to the internal data object buffer
   *
   * @return A pointer to the internal data object buffer
   */
  [[nodiscard]] __INLINE__ void *getData() const { return _buffer; }

  /**
   * Gets the size of the data object's internal data buffer
   *
   * @return The size of the data object's internal data buffer
   */
  [[nodiscard]] __INLINE__ size_t getSize() const { return _size; }

  private:

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
  dataObjectId_t _id;
};

} // namespace HiCR::deployer
