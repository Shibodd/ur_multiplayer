#include <iostream>
#include <thread>

#include <Ice/Ice.h>

#include <RaceControl.h>

#include <multiplayer/race_control_client_servant.hpp>

int main(int argc, char* argv[]) {
    Ice::InitializationData initData;
    initData.properties = Ice::createProperties(argc, argv);

    Ice::CommunicatorPtr communicator = Ice::initialize(initData);
    Ice::CommunicatorHolder communicatorHolder{ communicator };

    auto race_control = Ice::uncheckedCast<ur::multiplayer::idl::RaceControlPrx>(
        communicator->stringToProxy("racecontrol:udp -h localhost -p 3478")->ice_datagram()
    );

    // race_control->requestCircuitFlag(ur::multiplayer::idl::FlagId { 0 });
    race_control->requestVehicleFlag(ur::multiplayer::idl::ParticipantId { 0 }, ur::multiplayer::idl::FlagId { 0 });

    communicator->shutdown();
    communicator->waitForShutdown();
}