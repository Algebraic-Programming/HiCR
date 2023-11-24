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

// includes
#include <hicr/machineModel/deviceModel.hpp>
#include <hicr/machineModel/hostdev/hostDevice.hpp>

namespace HiCR
{

namespace machineModel
{

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
     * @param[in] types A vector of device types to enable or exclude from initialization;
     * probably will come from configuration
     */
    //MachineModel(std::vector<std::string> types)
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

    inline std::vector<machineModel::DeviceModel *> queryDevices()
    {
     std::vector<std::string> types = {"host"}; // temporary
     for (auto t : types)
     {
         machineModel::DeviceModel *devm = NULL;
         if (t == "host")
             devm = new machineModel::HostDevice();
//            else if (t == "ascend")
//            {
//                devm = new machineModel::DeviceModel();
//            }
//            else //temporary, FIXME
//                devm = new machineModel::DeviceModel();

         devm->initialize();
         _devices.push_back(devm);
     }

     return _devices;
    }

}; // class Base

} // namespace machineModel

} // namespace HiCR

