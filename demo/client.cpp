#include "VehicleState.h"
#include <chrono>
#include <iostream>
#include <thread>
#include <cmath>

#include <Ice/Ice.h>
#include <SFML/Graphics.hpp>

#include <Lobby.h>
#include <RaceControl.h>

#include <clock_sync/clock_offset_calculator.hpp>


#include <multiplayer/participant.hpp>

// Draw a vehicle rectangle
void drawVehicle(sf::RenderWindow &window, ur::multiplayer::idl::VehicleState state, sf::Color color) {
    sf::RectangleShape car(sf::Vector2f(50, 30));
    car.setFillColor(color);
    car.setRotation((180.0 / M_PI) * state.yaw);
    car.setOrigin(car.getSize().x / 2.f, car.getSize().y / 2.f);
    car.setPosition(state.position.x, state.position.y);
    window.draw(car);
}

void motion_model(
    ur::multiplayer::idl::VehicleState &state,
    float dt
) {
    const float L = 40.0f;
    const float max_speed = 500.f;

    const float accel_gain = 2000.0f;
    const float max_decel = 1000.f;
    const float max_accel = 1000.f;
    const float friction = 200.0f;

    // Update forward speed
    float acceleration = std::clamp(state.throttle * accel_gain, -max_decel, max_accel);
    acceleration -= std::clamp(state.velocity.x / dt, -friction, friction);

    state.velocity.x += acceleration * dt;
    state.velocity.x = state.velocity.x;

    // Lateral velocity = 0 in simple kinematic bicycle
    state.velocity.y = 0.f;

    // Update yaw
    state.yaw += (state.velocity.x / L) * std::tan(state.steering) * dt;
    // Optional clamp for yaw [-pi, pi]
    if (state.yaw > M_PI) state.yaw -= 2.f * M_PI;
    else if (state.yaw < -M_PI) state.yaw += 2.f * M_PI;

    // Update position
    state.position.x += std::cos(state.yaw) * state.velocity.x * dt;
    state.position.y += std::sin(state.yaw) * state.velocity.x * dt;
}

int main(int argc, char* argv[]) {
    Ice::InitializationData initData;
    initData.properties = Ice::createProperties(argc, argv);
    initData.properties->setProperty("ClientAdapter.PublishedEndpoints", "udp -h 192.168.1.65 -p 3477");

    Ice::CommunicatorPtr communicator = Ice::initialize(initData);
    Ice::CommunicatorHolder communicatorHolder{ communicator };

    Ice::ObjectAdapterPtr adapter = communicator->createObjectAdapterWithEndpoints("ClientAdapter", "udp -h 192.168.1.65 -p 3477");

    auto participant = std::make_shared<ur::multiplayer::Participant>();
    auto participant_proxy = Ice::uncheckedCast<ur::multiplayer::idl::ParticipantPrx>(adapter->addWithUUID(participant)).ice_datagram();

    adapter->activate();
    
    auto lobby = Ice::uncheckedCast<ur::multiplayer::idl::LobbyPrx>(
        communicator->stringToProxy("lobby:udp -h 192.168.1.71 -p 3478")->ice_datagram()
    );

    auto race_control = Ice::uncheckedCast<ur::multiplayer::idl::RaceControlPrx>(
        communicator->stringToProxy("racecontrol:udp -h 192.168.1.71 -p 3478")->ice_datagram()
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

    sf::RenderWindow window(sf::VideoMode(800, 600), "Mini Racing Demo");
    window.setFramerateLimit(60);

    std::mutex vs_mtx;
    ur::multiplayer::idl::VehicleState vs;
    vs.position.x = 0.0f;
    vs.position.y = 0.0f;
    vs.velocity.x = 0.0f;
    vs.velocity.y = 0.0f;
    vs.yaw = 0.0f;

    auto runner = std::thread([&]() {
        std::chrono::milliseconds interval { 50 };

        auto next = std::chrono::steady_clock::now();

        while (not stop_token) {
            std::this_thread::sleep_until(next);
            auto t = std::chrono::steady_clock::now();
            next = t + interval;

            auto offset = participant->clock_offset();

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

    // --- Main Render / Input Loop ---

    auto interval = std::chrono::milliseconds(16);
    auto dt = std::chrono::duration<float>(interval).count();
    while (window.isOpen() && !stop_token) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                stop_token = true;
                window.close();
            }
        }


        float throttle = sf::Keyboard::isKeyPressed(sf::Keyboard::Up) - sf::Keyboard::isKeyPressed(sf::Keyboard::Down);
        float steer = 0.3f * (sf::Keyboard::isKeyPressed(sf::Keyboard::Left) - sf::Keyboard::isKeyPressed(sf::Keyboard::Right));

        {
            std::scoped_lock lock(vs_mtx);
            vs.throttle = throttle;
            vs.steering = steer;
            motion_model(vs, dt);
        }

        window.clear(sf::Color::Black);

        // --- Render ---
        drawVehicle(window, vs, sf::Color::White);

        auto opponents = participant->opponents();
        auto t = std::chrono::system_clock::now();

        for (auto &op : opponents) {
            auto other_t = std::chrono::system_clock::time_point{ std::chrono::nanoseconds { op.vehicleState.stamp.nanoseconds } };

            auto dt = std::chrono::duration<float>(
                t - other_t
            ).count();

            motion_model(op.vehicleState, dt);
            drawVehicle(window, op.vehicleState, sf::Color::Red);
        }

        window.display();

        std::this_thread::sleep_for(interval); // ~60 FPS
    }

    // Wait until the communicator is shut down. Here, this occurs when the user presses Ctrl+C.
    communicator->waitForShutdown();
    runner.join();

    // std::cout << hello.message << std::endl;
}