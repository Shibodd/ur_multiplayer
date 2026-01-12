#include <Ice/Ice.h>
#include <future>
#include <iostream>

#include <Greeter.h>


int main(int argc, char* argv[]) {
    Ice::CommunicatorPtr communicator = Ice::initialize(argc, argv);
    Ice::CommunicatorHolder communicatorHolder{ communicator };

    ur::multiplayer::GreeterPrx greeter{communicator, "greeter:tcp -h localhost -p 4061"};
    std::string greeting = greeter->greet("suca");
    std::cout << greeting << std::endl;
}