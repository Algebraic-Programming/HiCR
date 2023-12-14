/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file machineModel/base.hpp
 * @brief Defines the API for interacting with the machine model
 * @author O. Korakitis
 * @date 05/10/2023
 */
#pragma once

#include <hicr/L2/machineModel/deviceModel.hpp>
// #include <hicr/L2/machineModel/hostdev/hostDevice.hpp>

namespace HiCR
{

namespace L2
{

namespace machineModel
{

/**
 * Base implementation of the machine model
 */
class Base
{
  protected:

  /**
   * Keep the various devices discovered by the Backends
   */
  std::vector<machineModel::DeviceModel *> _devices;

  // Scheduling domains - peers

  public:

  /**
   * Constructor for a MachineModel object; a program will have one such instance.
   * It initializes the device backends, including querying of resources.
   * TODO: Parameterize:
   */
  Base()
  {
  }

  ~Base()
  {
    //        for (auto d : _devices)
    //        {
    //            d->shutdown();
    //            delete d;
    //        }
  }

  /**
   * This function detects the local devices and stores their information
   */
  __USED__ inline void queryDevices()
  {
    // std::vector<std::string> types = {"host"}; // temporary
    // for (auto t : types)
    // {
      // machineModel::DeviceModel *devm = NULL;
      // if (t == "host")
        // devm = new machineModel::HostDevice();
      //            else if (t == "ascend")
      //            {
      //                devm = new machineModel::DeviceModel();
      //            }
      //            else //temporary, FIXME
      //                devm = new machineModel::DeviceModel();

      // // devm->initialize();
      // _devices.push_back(devm);
    // }
  }

}; // class Base

} // namespace machineModel

} // namespace L2

} // namespace HiCR
