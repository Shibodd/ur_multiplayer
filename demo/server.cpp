#include <iostream>

#include <Ice/Ice.h>

#include <multiplayer/server.hpp>

int main(int argc, char* argv[])
{
    // CtrlCHandler is a helper class that handles Ctrl+C and similar signals. It must be constructed at the beginning
    // of the program, before creating an Ice communicator or starting any thread.
    Ice::CtrlCHandler ctrlCHandler;

    // Create an Ice communicator. We'll use this communicator to create an object adapter.
    Ice::CommunicatorPtr communicator = Ice::initialize(argc, argv);

    // Make sure the communicator is destroyed at the end of this scope.
    Ice::CommunicatorHolder communicatorHolder{communicator};

    // Create an object adapter that listens for incoming requests and dispatches them to servants.
    auto adapter = communicator->createObjectAdapterWithEndpoints("ServerAdapter", "udp -h localhost -p 3478");

    // Register the Chatbot servant with the adapter.
    adapter->add(std::make_shared<ur::multiplayer::LobbyServant>(), Ice::Identity{"lobby"});

    // Start dispatching requests.
    adapter->activate();
    std::cout << "Listening on port 3478..." << std::endl;

    // Shut down the communicator when the user presses Ctrl+C.
    ctrlCHandler.setCallback(
        [communicator](int signal)
        {
            std::cout << "Caught signal " << signal << ", shutting down..." << std::endl;
            communicator->shutdown();
        });

    // Wait until the communicator is shut down. Here, this occurs when the user presses Ctrl+C.
    communicator->waitForShutdown();

    return 0;
}