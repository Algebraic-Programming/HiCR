#pragma once

#include <cstdint>
#include <memory>
#include <hicr/core/L0/instance.hpp>
#include <hicr/core/definitions.hpp>
#include <hicr/core/exceptions.hpp>
#include "../dataObject.hpp"

namespace HiCR::deployer::dataObject
{

/**
 * Prototype implementation of the data object class for the host (single instance) deployer mode
 */
class Local final : public HiCR::deployer::DataObject
{
  public:

  /**
   * Standard data object constructor
   *
   * @param[in] buffer The internal buffer to use for the data object
   * @param[in] size The size of the internal buffer
   * @param[in] id Identifier for the new data object
   * @param[in] instanceId Identifier of the instance owning the data object
   * @param[in] seed random application seed  
   */
  Local(void *const buffer, const std::size_t size, const dataObjectId_t id, const HiCR::L0::Instance::instanceId_t instanceId, const HiCR::L0::Instance::instanceId_t seed)
    : DataObject(buffer, size, id, instanceId, seed)
  {}

  ~Local() = default;

  /**
   * Exposes a data object to be obtained (stealed) by another instance
   */
  __INLINE__ void publish() override
  {
    // Here an exception is must be produced since the application is exposing this data object to other non-existent instances, leading to a potential deadlock
    // HICR_THROW_LOGIC("Attempting to publish a data object when using the host (single instace) deployer mode.");
  }

  __INLINE__ void unpublish() override {}

  __INLINE__ bool tryRelease() override
  {
    // Here an exception is must be produced since the application is releasing this data object to other non-existent instances, leading to a potential deadlock
    // HICR_THROW_LOGIC("Attempting to release a data object when using the host (single instace) deployer mode.");

    return false;
  }

  __INLINE__ void get(HiCR::L0::Instance::instanceId_t currentInstanceId, HiCR::L0::Instance::instanceId_t seed) override
  {
    // An exception is produced here, since as there is no other instance active, this object will never be retrieved
    HICR_THROW_LOGIC("Attempting to get a data object when using the host (single instance) deployer mode.");
  }
};

} // namespace HiCR::deployer::dataObject
