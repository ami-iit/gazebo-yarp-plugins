/*
 * Copyright (C) 2013-2015 Fondazione Istituto Italiano di Tecnologia RBCS & iCub Facility & ADVR
 * Authors: see AUTHORS file.
 * CopyPolicy: Released under the terms of the LGPLv2.1 or any later version, see LGPL.TXT or LGPL3.TXT
 */


#include "Camera.hh"
#include "CameraDriver.h"
#include <GazeboYarpPlugins/Handler.hh>
#include <GazeboYarpPlugins/common.h>
#include <GazeboYarpPlugins/ConfHelpers.hh>

#include <gazebo/sensors/CameraSensor.hh>
#include <yarp/dev/PolyDriver.h>
#include <yarp/os/Log.h>
#include <yarp/os/LogStream.h>

GZ_REGISTER_SENSOR_PLUGIN(gazebo::GazeboYarpCamera)


namespace gazebo {

GazeboYarpCamera::GazeboYarpCamera() : CameraPlugin(), m_yarp(), m_deviceRegistered(false)
{
}

GazeboYarpCamera::~GazeboYarpCamera()
{
    if (m_deviceRegistered) {
        GazeboYarpPlugins::Handler::getHandler()->removeDevice(m_scopedDeviceName);
        m_deviceRegistered = false;
    }
    m_cameraDriver.close();
    GazeboYarpPlugins::Handler::getHandler()->removeSensor(m_sensorName);
}

void GazeboYarpCamera::Load(sensors::SensorPtr _sensor, sdf::ElementPtr _sdf)
{
    if (!m_yarp.checkNetwork(GazeboYarpPlugins::yarpNetworkInitializationTimeout)) {
        yError() << "GazeboYarpCamera::Load error: yarp network does not seem to be available, is the yarpserver running?";
        return;
    }


    if (!_sensor) {
        gzerr << "GazeboYarpCamera plugin requires a CameraSensor.";
        return;
    }

    _sensor->SetActive(true);

    // Add my gazebo device driver to the factory.
    ::yarp::dev::Drivers::factory().add(new ::yarp::dev::DriverCreatorOf< ::yarp::dev::GazeboYarpCameraDriver>
                                        ("gazebo_camera", "grabber", "GazeboYarpCameraDriver"));


    //Getting .ini configuration file from sdf
    bool configuration_loaded = GazeboYarpPlugins::loadConfigSensorPlugin(_sensor,_sdf,m_parameters);

    if (!configuration_loaded) {
        return;
    }

    m_sensorName = _sensor->ScopedName();

    m_sensor = (gazebo::sensors::CameraSensor*)_sensor.get();
    if(m_sensor == NULL)
    {
        yDebug() << "m_sensor == NULL";
    }


    // Don't forget to load the camera plugin!!!! ???
//    CameraPlugin::Load(_sensor, _sdf);

    yDebug() << "GazeboYarpCamera Plugin: sensor scoped name is " << m_sensorName.c_str();
    //Insert the pointer in the singleton handler for retriving it in the yarp driver
    GazeboYarpPlugins::Handler::getHandler()->setSensor(_sensor.get());

    m_parameters.put(YarpScopedName.c_str(), m_sensorName.c_str());

    // If the device is not specified in the configuration passed, specify it to gazebo_camera
    if (!m_parameters.check("device")) {
        m_parameters.put("device", "gazebo_camera");
    }

    //Open the driver
    if (m_cameraDriver.open(m_parameters)) {
        yInfo() << "Loaded GazeboYarpCamera Plugin correctly";
    } else {
        yError() << "GazeboYarpCamera Plugin Load failed: error in opening yarp driver";
    }

    m_cameraDriver.view(iFrameGrabberImage);
    if(iFrameGrabberImage == NULL)
    {
        yError() << "Unable to get the iFrameGrabberImage interface from the device";
        return;
    }
    
    // Register the device with the given name
    if(!m_parameters.check("yarpDeviceName"))
    {
        m_scopedDeviceName = m_sensorName + "::" "camera";
    }
    else
    {
        m_scopedDeviceName = m_sensorName + "::" + m_parameters.find("yarpDeviceName").asString();
    }

    if(!GazeboYarpPlugins::Handler::getHandler()->setDevice(m_scopedDeviceName, &m_cameraDriver))
    {
        yError()<<"GazeboYarpCamera: failed setting scopedDeviceName(=" << m_scopedDeviceName << ")";
        return;
    }
    m_deviceRegistered = true;
    yInfo() << "GazeboYarpCamera: Register YARP device with instance name:" << m_scopedDeviceName;

}

}
