
/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file machineModel/model.hpp
 * @brief Definition of the HiCR machine Model class
 * @author O. Korakitis & S. M. Martin
 * @date 24/11/2023
 */

#pragma once

#include <hicr/common/definitions.hpp>
#include <hicr/common/exceptions.hpp>
#include <hicr/machineModel/base.hpp>

namespace HiCR
{

namespace machineModel
{

/**
 * Class definition for the HiCR machine model.
 *
 * This class provides an overview of the entire system (compute and memory elements) and their connectivity
 */
class Model : public machineModel::Base
{
  public:

  /**
   * This function creates a new empty machine model.
   *
   * The internal data can be updated upon explicit initialization
   */
  Model() : machineModel::Base()
  {  }

  /**
   * This function creates a new machine model by directly deserializing an input.
   *
   * @param[in] serialData The serial representation of the model's internal state to deserialize (parse)
   */
  Model(const std::string &serialData) : machineModel::Base()
  {
    deserialize(serialData);
  }

  ~Model() = default;

  /**
   * This function uses all detected backends to create/update the instance-local view of the machine model
   */
  __USED__ inline void update()
  {
   queryDevices();
  }

  /**
   * This function serializes the internal machine model into a compact, transmissible data string}
   *
   * @return A string containing the serialized internal representation of the model
   */
  __USED__ inline std::string serialize() const
  {
   return _devices[0]->jSerialize().dump();
  }

  /**
   * This function takes a serialized machine model binary data and creates the correpsonding objects inside this class
   *
   * @param[in] serialData The serial representation of the model's internal state to deserialize (parse)
   */
  __USED__ inline void deserialize(const std::string &serialData)
  {
   nlohmann::json data = nlohmann::json::parse(serialData);
   auto d = new machineModel::HostDevice(data);
   _devices.clear();
   _devices.push_back(d);
  }

  /**
   * This function serialized the internal model into a string that can be printed to screen or log
   *
   * @return A log/screen friendly representation of the internal state
   *
   */
  __USED__ inline std::string stringify() const
  {
   return _devices[0]->jSerialize().dump(2);
  }

  private:

  std::string _internalData;
};

} // namespace machineModel

} // namespace HiCR
