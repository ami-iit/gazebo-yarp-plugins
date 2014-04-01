/*
 * Copyright (C) 2007-2013 Istituto Italiano di Tecnologia ADVR & iCub Facility
 * Authors: Enrico Mingo, Alessio Rocchi, Mirko Ferrati, Silvio Traversaro and Alessandro Settimi
 * CopyPolicy: Released under the terms of the LGPLv2.1 or later, see LGPL.TXT
 */
#include "gazebo_yarp_plugins/ControlBoard.hh"
#include "gazebo_yarp_plugins/Handler.hh"
#include "gazebo_yarp_plugins/ControlBoardDriver.h"

using namespace std;
namespace gazebo
{
    
GZ_REGISTER_MODEL_PLUGIN(GazeboYarpControlBoard)

    GazeboYarpControlBoard::GazeboYarpControlBoard() : _yarp(), _iWrap(0)
    {

    }

    GazeboYarpControlBoard::~GazeboYarpControlBoard()
    {
        if(_iWrap) { _iWrap->detachAll(); _iWrap = 0; }
        if( _wrapper.isValid() ) _wrapper.close();
        if( _controlBoard.isValid() ) _controlBoard.close();
        GazeboYarpPlugins::Handler::getHandler()->removeRobot(_robotName);
        std::cout<<"Goodbye!"<<std::endl;
    }

    /**
     * Saves the gazebo pointer, creates the device driver
     */
    void GazeboYarpControlBoard::Load(physics::ModelPtr _parent, sdf::ElementPtr _sdf)
    {
        if( !_yarp.checkNetwork() ) { 
            std::cerr << "GazeboYarpControlBoard::Load error: yarp network does not seem to be available, is the yarpserver running?"<<std::endl;
            return;
        }
        std::cout<<"*** GazeboYarpControlBoard plugin started ***"<<std::endl;
        
        if (!_parent)
        {
            gzerr << "GazeboYarpControlBoard plugin requires a parent.\n";
            return;
        }
        
        _robotName = _parent->GetScopedName();
        GazeboYarpPlugins::Handler::getHandler()->setRobot(get_pointer(_parent));

        // Add the gazebo_controlboard device driver to the factory.
        yarp::dev::Drivers::factory().add(new yarp::dev::DriverCreatorOf<yarp::dev::GazeboYarpControlBoardDriver>
                                          ("gazebo_controlboard", "controlboardwrapper2", "GazeboYarpControlBoardDriver"));

        std::cout<<"\n Initting Wrapper\n"<<std::endl;
        //Getting .ini configuration file from sdf
        bool configuration_loaded = false;



        yarp::os::Bottle wrapper_group;
        yarp::os::Bottle driver_group;
        if(_sdf->HasElement("yarpConfigurationFile") )
        {
            std::string ini_file_name = _sdf->Get<std::string>("yarpConfigurationFile");
            std::string ini_file_path = gazebo::common::SystemPaths::Instance()->FindFileURI(ini_file_name);

            if( ini_file_path != "" && _parameters.fromConfigFile(ini_file_path.c_str()) )
            {
                std::cout << "GazeboYarpControlBoard: Found yarpConfigurationFile: loading from " << ini_file_path << std::endl; 
                _parameters.put("gazebo_ini_file_path",ini_file_path.c_str());
            
                //std::cout << "<<<<<< Just read file\n " << _parameters.toString() << "\n>>>>>>\n";
                wrapper_group = _parameters.findGroup("WRAPPER");
                if(wrapper_group.isNull())
                {
                    printf("GazeboYarpControlBoard::Load  Error: [WRAPPER] group not found in config file\n");
                    return;
                }

                configuration_loaded = true;
            }
            
        }
        if( !configuration_loaded )
        {
            std::cout << "GazeboYarpControlBoard: File .ini not found, quitting\n" << std::endl;
            return;
        }

        _wrapper.open(wrapper_group);
    
        if (!_wrapper.isValid())
            fprintf(stderr, "GazeboYarpControlBoard: wrapper did not open\n");
        else
            fprintf(stderr, "GazeboYarpControlBoard: wrapper opened correctly\n");

        if( !_wrapper.view(_iWrap) )
        {
            printf("Wrapper interface not found\n");
        }

        yarp::os::Bottle *netList = wrapper_group.find("networks").asList();
        if(netList->isNull())
        {
            printf("GazeboYarpControlBoard ERROR, net list to attach to was not found, exiting\n");
            _wrapper.close();
            // _controlBoard.close();
            return;
        }

        yarp::dev::PolyDriverList p;

        for(int n=0; n<netList->size(); n++)
        {
            yarp::os::ConstString driverName( netList->get(n).asString().c_str());


            driver_group = _parameters.findGroup(driverName.c_str());
            if(driver_group.isNull())
            {
                printf("GazeboYarpControlBoard::Load  Error: [%s] group not found in config file\n", driverName.c_str());
                return;
            }

            _parameters.put("name", driverName.c_str());
            _parameters.fromString(driver_group.toString(), false);
            _parameters.put("robotScopedName", _robotName);
            std::cout << "GazeboYarpControlBoard: setting robotScopedName " << _robotName << std::endl;
             //std::cout << "before open: params are " << _parameters.toString() << std::endl;

            if(_sdf->HasElement("initialConfiguration") )
            {
                //std::cout<<"Found initial Configuration: "<<std::endl;
                std::string configuration_s = _sdf->Get<std::string>("initialConfiguration");
                _parameters.put("initialConfiguration", configuration_s.c_str());
                //std::cout<<configuration_s<<std::endl;
            }

            _controlBoard.open(_parameters);

            if (!_controlBoard.isValid())
                fprintf(stderr, "controlBoard did not open\n");
            else
                printf("controlBoard opened correctly\n");

            p.push(&_controlBoard, netList->get(n).asString().c_str());
        }


        if(!_iWrap || !_iWrap->attachAll(p))
        {
            printf("GazeboYarpControlBoard: Error while attaching wrapper to device\n");
            _wrapper.close();
            _controlBoard.close();
            return;
        }

        printf("Device initialized correctly, now sitting and waiting cause I am just the main of the yarp device, and the robot is linked to the onUpdate event of gazebo\n");
        std::cout<<"Loaded GazeboYarpControlBoard Plugin"<<std::endl;
    }


}
