#include "VehicleState.h"
#include <iostream>
#include <thread>

#include <Ice/Ice.h>

#include <Lobby.h>
#include <RaceControl.h>

#include <clock_sync/clock_offset_calculator.hpp>

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

    Ice::CtrlCHandler ctrlCHandler;
    std::atomic_bool stop_token = false;

    // Shut down the communicator when the user presses Ctrl+C.
    ctrlCHandler.setCallback(
        [communicator, &stop_token](int signal)
        {
            std::cout << "Caught signal " << signal << ", shutting down..." << std::endl;
            communicator->shutdown();
            stop_token = true;
        });

    do {
        lobby->join("", participant_proxy);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    } while (not participant->has_joined());

    std::mutex vs_mtx;
    ur::multiplayer::idl::VehicleState vs;

    auto runner = std::thread([&]() {
        std::chrono::milliseconds interval { 50 };

        auto next = std::chrono::steady_clock::now();

        while (not stop_token) {
            std::this_thread::sleep_until(next);
            auto t = std::chrono::steady_clock::now();
            next = t + interval;

            auto offset = participant->clock_offset();

            if (offset) {
                std::cout << offset->count() << std::endl;
            }

            ur::multiplayer::idl::VehicleState vehicle_state;
            {
                std::scoped_lock lock(vs_mtx);
                vehicle_state = vs;
            }

            auto proxy = participant->server_proxy();
            auto tx_stamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
                clock_sync::ClockOffsetCalculator::Clock::now().time_since_epoch()
            );
            proxy->hrUpdate({ tx_stamp.count() }, vehicle_state);
        }
    });

    // race_control.requestCircuitFlag(ur::multiplayer::idl::FlagId { 5 });

    // Wait until the communicator is shut down. Here, this occurs when the user presses Ctrl+C.
    communicator->waitForShutdown();
    runner.join();

    // std::cout << hello.message << std::endl;
}