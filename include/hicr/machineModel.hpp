/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file machineModel.hpp
 * @brief Defines the API for interacting with the machine model
 * @author O. Korakitis
 * @date 05/10/2023
 */
#pragma once

// includes
#include <hicr/machineModel/deviceModel.hpp>
#include <hicr/machineModel/hostDevice.hpp>

namespace HiCR
{

class MachineModel
{
  private:
    /**
     * Keep the various devices discovered by the Backends
     */
    std::vector<DeviceModel *> _devices;

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
    MachineModel()
    {
        std::vector<std::string> types = {"host"}; // temporary
        for (auto t : types)
        {
            DeviceModel *devm;
            if (t == "host")
                devm = new HostDevice();
            else //temporary, FIXME
                devm = new DeviceModel();

            devm->initialize();
            _devices.push_back(devm);
        }
    }

    ~MachineModel()
    {
        for (auto d : _devices)
        {
            d->shutdown();
            delete d;
        }
    }

    inline std::vector<DeviceModel *> queryDevices()  const //getMachineModel()
    {
        // Get value by copy
        auto ret = _devices;
        return ret;
    }

}; // class MachineModel


} // namespace HiCR

