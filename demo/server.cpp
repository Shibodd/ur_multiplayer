#include <iostream>
#include <thread>

#include <Ice/Ice.h>

#include <multiplayer/lobby.hpp>
#include <multiplayer/race_control.hpp>

using namespace ur::multiplayer;

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

    // Register the servants with the adapter.
    auto lobby = std::make_shared<LobbyServant>();
    auto racecontrol = std::make_shared<RaceControlServant>();

    adapter->add(lobby, Ice::Identity{"lobby"});
    adapter->add(racecontrol, Ice::Identity{"racecontrol"});

    // Start dispatching requests.
    adapter->activate();
    std::cout << "Listening on port 3478..." << std::endl;

    std::atomic_bool stop_token = false;

    auto thread = std::thread([&]() {
        auto interval = std::chrono::milliseconds { 50 };
        auto timeout = std::chrono::milliseconds { 2500 };
        auto next = std::chrono::steady_clock::now();

        while (not stop_token) {
            std::this_thread::sleep_until(next);
            auto t = std::chrono::steady_clock::now();
            next = t + interval;

            Participants participants(*lobby);

            idl::Opponents world_state;
            world_state.reserve(participants.size());

            auto it = participants.begin();
            auto end = participants.end();

            while (it != end) {
                if (auto ptr = it->second.lock()) {
                    auto id = ptr->participant_id();

                    if ((t - ptr->last_update()) > timeout) {
                        std::cout << "Participant " << id << " timed out." << std::endl;
                        ptr.reset();
                        it = participants.erase(it);
                        adapter->remove(Ice::Identity { std::to_string(id), "server" });
                        continue;
                    }

                    world_state.push_back({
                        idl::ParticipantId { static_cast<idl_participant_id_t>(id) },
                        ptr->get_state()
                    });
                }

                ++it;
            }

            it = participants.begin();

            for (it = participants.begin(); it != end; ++it) {
                if (auto ptr = it->second.lock()) {
                    auto idl_id = static_cast<idl_participant_id_t>(ptr->participant_id());

                    ur::multiplayer::idl::Duration rx_delay;

                    if (auto meas = ptr->min_rx_delay()) {
                        rx_delay.value = std::chrono::duration_cast<std::chrono::nanoseconds>(meas->value).count();
                    } else {
                        rx_delay.value = std::numeric_limits<decltype(rx_delay.value)>::max();
                    }

                    // Avoid sending the current player state
                    std::optional<idl::Opponent> this_state;

                    {
                        auto this_state_it = std::find_if(world_state.begin(), world_state.end(), [idl_id](const idl::Opponent& x) {
                            return x.participantId.value == idl_id;
                        });
    
                        if (this_state_it != world_state.end()) {
                            this_state = *this_state_it;
                            world_state.erase(this_state_it);
                        }
                    }

                    ptr->participant_proxy().hrUpdate(
                        idl::Timestamp { std::chrono::duration_cast<std::chrono::nanoseconds>(
                            std::chrono::steady_clock::now().time_since_epoch()
                        ).count() },
                        rx_delay,
                        world_state,
                        idl::FlagId { 0 },
                        idl::FlagId { 0 },
                        idl::ScheduledFlag { idl::FlagId { 0 }, idl::Timestamp { 0 }}
                    );

                    // Restore the current player state so that the others receive it
                    if (this_state.has_value()) {
                        world_state.push_back(*this_state);
                    }
                }
            }
        }
    });


    // Shut down the communicator when the user presses Ctrl+C.
    ctrlCHandler.setCallback(
        [communicator, &stop_token](int signal)
        {
            std::cout << "Caught signal " << signal << ", shutting down..." << std::endl;
            communicator->shutdown();
            stop_token = true;
        });

    // Wait until the communicator is shut down. Here, this occurs when the user presses Ctrl+C.
    communicator->waitForShutdown();

    thread.join();

    return 0;
}