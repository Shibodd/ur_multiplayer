#include "FlagId.h"
#include <iostream>
#include <thread>

#include <Ice/Ice.h>

#include <Lobby.h>
#include <RaceControl.h>

#include <multiplayer/participant.hpp>

int main(int argc, char* argv[]) {
    Ice::CommunicatorPtr communicator = Ice::initialize(argc, argv);
    Ice::CommunicatorHolder communicatorHolder{ communicator };

    Ice::ObjectAdapterPtr adapter = communicator->createObjectAdapterWithEndpoints("ClientAdapter", "udp -h localhost -p 3477");

    auto participant = std::make_shared<ur::multiplayer::Participant>();
    auto participant_proxy = Ice::uncheckedCast<ur::multiplayer::idl::ParticipantPrx>(adapter->addWithUUID(participant)).ice_datagram();

    adapter->activate();
    
    auto lobby = Ice::uncheckedCast<ur::multiplayer::idl::LobbyPrx>(
        communicator->stringToProxy("lobby:udp -h localhost -p 3478")->ice_datagram()
    );

    auto race_control = Ice::uncheckedCast<ur::multiplayer::idl::RaceControlPrx>(
        communicator->stringToProxy("racecontrol:udp -h localhost -p 3478")->ice_datagram()
    );


    do {
        lobby->join("", participant_proxy);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    } while (not participant->has_joined());

    race_control.requestCircuitFlag(ur::multiplayer::idl::FlagId { 5 });

    /*
    Ice::CtrlCHandler ctrlCHandler;

    // Shut down the communicator when the user presses Ctrl+C.
    ctrlCHandler.setCallback(
        [communicator](int signal)
        {
            std::cout << "Caught signal " << signal << ", shutting down..." << std::endl;
            communicator->shutdown();
        });

    // Wait until the communicator is shut down. Here, this occurs when the user presses Ctrl+C.
    communicator->waitForShutdown();
    */

    // std::cout << hello.message << std::endl;
}