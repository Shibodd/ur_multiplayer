#include "multiplayer/race_control.hpp"
#include <Ice/CtrlCHandler.h>
#include <Ice/Properties.h>
#include <iostream>
#include <thread>


#include <Ice/Ice.h>

#include <RaceControl.h>

#include <multiplayer/race_control_client_servant.hpp>

int main(int argc, char* argv[]) {
    Ice::InitializationData initData;
    initData.properties = std::make_shared<Ice::Properties>(argc, argv);

    Ice::CommunicatorPtr communicator = Ice::initialize(initData);
    Ice::CommunicatorHolder communicatorHolder{ communicator };

    auto race_control = Ice::uncheckedCast<ur::multiplayer::idl::RaceControlPrx>(
        communicator->stringToProxy("racecontrol:" + initData.properties->getProperty("Demo.Remote"))->ice_datagram()
    );

    auto flag_id = ur::multiplayer::idl::FlagId {
        static_cast<ur::multiplayer::flag_id_t>(
            initData.properties->getPropertyAsInt("RcClient.FlagId")
        )
    };

    auto flag_type = initData.properties->getProperty("RcClient.FlagType");
    if (flag_type == "vehicle") {
        auto id = initData.properties->getPropertyAsInt("RcClient.VehicleId");
        std::cout << "Vehicle " << id << " flag " <<flag_id << std::endl;
        race_control->requestVehicleFlag(
            ur::multiplayer::idl::ParticipantId {static_cast<decltype(ur::multiplayer::idl::ParticipantId::value)>(id)},
            flag_id
        );
    } else if (flag_type == "circuit") {
        std::cout << "Circuit " << flag_id << std::endl;
        race_control->requestCircuitFlag(flag_id);
    } else {
        std::cerr << "no such flag type" << std::endl;
    }

    communicator->shutdown();
    communicator->waitForShutdown();
}