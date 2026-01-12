// Copyright (c) ZeroC, Inc.

#include <Ice/Ice.h>
#include <iostream>
#include "Greeter.h"

using namespace std;

namespace ur {
namespace multiplayer {

class MyServant : public ur::multiplayer::Greeter
{
public:
    std::string greet(std::string aName, const Ice::Current& aSuca) override {
        return aName + ", suca!";
    }
};

} // namespace multiplayer
} // namespace ur


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
    auto adapter = communicator->createObjectAdapterWithEndpoints("GreeterAdapter", "tcp -p 4061");

    // Register the Chatbot servant with the adapter.
    adapter->add(make_shared<ur::multiplayer::MyServant>(), Ice::Identity{"greeter"});

    // Start dispatching requests.
    adapter->activate();
    cout << "Listening on port 4061..." << endl;

    // Shut down the communicator when the user presses Ctrl+C.
    ctrlCHandler.setCallback(
        [communicator](int signal)
        {
            cout << "Caught signal " << signal << ", shutting down..." << endl;
            communicator->shutdown();
        });

    // Wait until the communicator is shut down. Here, this occurs when the user presses Ctrl+C.
    communicator->waitForShutdown();

    return 0;
}