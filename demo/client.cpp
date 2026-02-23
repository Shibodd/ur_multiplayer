#include "Lobby.h"
#include "Participant.h"
#include <Ice/Ice.h>
#include <Ice/ProxyFunctions.h>
#include <future>
#include <iostream>

#include <multiplayer/participant.hpp>

int main(int argc, char* argv[]) {
    Ice::CommunicatorPtr communicator = Ice::initialize(argc, argv);
    Ice::CommunicatorHolder communicatorHolder{ communicator };

    Ice::ObjectAdapterPtr adapter = communicator->createObjectAdapterWithEndpoints("ClientAdapter", "udp -h localhost -p 3477");

    auto participant = Ice::uncheckedCast<ur::multiplayer::idl::ParticipantPrx>(adapter->addWithUUID(std::make_shared<ur::multiplayer::Participant>()));

    adapter->activate();

    
    auto lobby = Ice::uncheckedCast<ur::multiplayer::idl::LobbyPrx>(
        communicator->stringToProxy("lobby:udp -h localhost -p 3478")->ice_datagram()
    );

    for (int i = 0; i < 5; ++i) {
        lobby->join("test", participant);
    }

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

    // std::cout << hello.message << std::endl;
}