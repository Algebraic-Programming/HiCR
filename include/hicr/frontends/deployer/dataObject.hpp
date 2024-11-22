#pragma once

#include <cstdint>
#include <memory>
#include <mpi.h>
#include <hicr/core/L0/instance.hpp>

namespace HiCR::deployer
{

/**
 * Prototype implementation of the data object class
 */
class DataObject
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
  DataObject(void *const buffer, const size_t size, const dataObjectId_t id, const HiCR::L0::Instance::instanceId_t instanceId, const HiCR::L0::Instance::instanceId_t seed)
    : _buffer(buffer),
      _instanceId(instanceId),
      _size(size),
      _id(id){};
  ~DataObject() = default;

  /**
   * Exposes a data object to be obtained (stealed) by another instance
   */
  virtual void publish() = 0;
  /**
   * Mark the object available for publication again
   */
  virtual void unpublish() = 0;

  /**
   * Tries to release a previously published data object any instance that wants to take it
   *
   * @return True, if the data object was successfully release (copied to another instance), or was already released; false, if nobody claimed the data object
   */
  virtual bool tryRelease() = 0;

  /**
   * Obtains a data object from a remote instance, based on its id
   *
   * This function will stall until and unless the specified remote instance published the given data object
   *
   * @param[in] currentInstanceId Id of the instance that will own the data object
   * @param[in] seed unique random seed 
   */
  virtual void get(HiCR::L0::Instance::instanceId_t currentInstanceId, HiCR::L0::Instance::instanceId_t seed) = 0;

  /**
   * Gets the data object id
   *
   * @return The data object id
   */
  [[nodiscard]] dataObjectId_t getId() const { return _id; }

  /**
   * Set the data object id
   * @param[in] id
   */
  void setId(const dataObjectId_t id) { _id = id; }

  /**
   * Get data object instance id that owns it
   *
   * @return The instance id
   */
  [[nodiscard]] HiCR::L0::Instance::instanceId_t getInstanceId() const { return _instanceId; }

  /**
   * Set the data object instance id
   * @param[in] instanceId
   */
  void setInstanceId(const HiCR::L0::Instance::instanceId_t instanceId) { _instanceId = instanceId; }

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

  protected:

  /**
   * Set the data object internal buffer
   *
   * @param[in] buffer
   */
  __INLINE__ void setData(void *const buffer) { _buffer = buffer; }

  private:

  /**
   * The data object's internal data buffer
   */
  void *_buffer;

  /**
   * The data object's source isntance id
   */
  HiCR::L0::Instance::instanceId_t _instanceId;

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
