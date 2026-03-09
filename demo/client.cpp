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
    std::chrono::steady_clock::time_point t
) {
    constexpr float L = 40.0f;

    constexpr float ACCEL_GAIN = 2000.0f;
    constexpr float MAX_DECEL = 1000.f;
    constexpr float MAX_ACCEL = 1000.f;
    constexpr float FRICTION = 300.f;

    constexpr std::chrono::nanoseconds DT_MAX = std::chrono::milliseconds { 10 };

    auto to_integrate = t - std::chrono::steady_clock::time_point { std::chrono::nanoseconds { state.stamp.nanoseconds } };

    while (to_integrate > std::chrono::nanoseconds::zero()) {
        std::chrono::nanoseconds delta_t = std::min(to_integrate, DT_MAX);
        
        to_integrate -= delta_t;

        float dt = std::chrono::duration<float>{ delta_t }.count();

        float acceleration = std::clamp(state.throttle * ACCEL_GAIN, -MAX_DECEL, MAX_ACCEL);
        state.vx += acceleration * dt;

        float friction = std::copysign(FRICTION, state.vx);
        if ((std::abs(friction) * dt) > std::abs(state.vx)) {
            state.vx = 0.0;
        } else {
            state.vx -= friction * dt;
        }

        // Update forward speed
        state.vx = state.vx;

        // Update yaw
        state.yaw += (state.vx / L) * std::tan(state.steering) * dt;
        // Optional clamp for yaw [-pi, pi]
        if (state.yaw > M_PI) state.yaw -= 2.f * M_PI;
        else if (state.yaw < -M_PI) state.yaw += 2.f * M_PI;

        // Update position
        state.position.x += std::cos(state.yaw) * state.vx * dt;
        state.position.y += std::sin(state.yaw) * state.vx * dt;
    }

    state.stamp.nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(
        t.time_since_epoch()
    ).count();
}

int main(int argc, char* argv[]) {
    Ice::InitializationData initData;
    initData.properties = Ice::createProperties(argc, argv);
    initData.properties->setProperty("ClientAdapter.PublishedEndpoints", initData.properties->getProperty("Demo.Local"));
    

    Ice::CommunicatorPtr communicator = Ice::initialize(initData);
    Ice::CommunicatorHolder communicatorHolder{ communicator };

    Ice::ObjectAdapterPtr adapter = communicator->createObjectAdapterWithEndpoints("ClientAdapter", initData.properties->getProperty("Demo.Local"));

    auto participant = std::make_shared<ur::multiplayer::Participant>();
    auto participant_proxy = Ice::uncheckedCast<ur::multiplayer::idl::ParticipantPrx>(adapter->addWithUUID(participant)).ice_datagram();

    adapter->activate();
    
    auto lobby = Ice::uncheckedCast<ur::multiplayer::idl::LobbyPrx>(
        communicator->stringToProxy("lobby:" + initData.properties->getProperty("Demo.Remote"))->ice_datagram()
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
    window.setFramerateLimit(100);

    std::mutex vs_mtx;
    ur::multiplayer::idl::VehicleState vs;
    vs.position.x = 0.0f;
    vs.position.y = 0.0f;
    vs.vx = 0.0f;
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

            if (offset) {
                vehicle_state.stamp.nanoseconds += std::chrono::duration_cast<std::chrono::nanoseconds>(*offset).count();
            }

            auto proxy = participant->server_proxy();
            auto tx_stamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::steady_clock::now().time_since_epoch()
            );
            proxy->hrUpdate({ tx_stamp.count() }, vehicle_state);
        }
    });

    // --- Main Render / Input Loop ---

    std::array cflag_color_map = {
        sf::Color::Green,
        sf::Color::Yellow,
        sf::Color::Red,
    };

    std::array vflag_color_map = {
        sf::Color::Green,
        sf::Color::Blue
    };

    while (window.isOpen() && !stop_token) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                stop_token = true;
                window.close();
            }
        }

        auto t = std::chrono::steady_clock::now();

        float throttle = sf::Keyboard::isKeyPressed(sf::Keyboard::Up) - sf::Keyboard::isKeyPressed(sf::Keyboard::Down);
        float steer = 0.3f * (sf::Keyboard::isKeyPressed(sf::Keyboard::Right) - sf::Keyboard::isKeyPressed(sf::Keyboard::Left));

        {
            std::scoped_lock lock(vs_mtx);
            vs.throttle = throttle;
            vs.steering = steer;
            motion_model(vs, t);
        }

        window.clear(sf::Color::Black);

        // --- Render ---
        drawVehicle(window, vs, sf::Color::White);

        auto opponents = participant->opponents();

        for (auto &op : opponents) {
            motion_model(op.vehicleState, t);
            drawVehicle(window, op.vehicleState, sf::Color::Red);
        }

        auto flags = participant->flags(t);

        const sf::Vector2f FLAG_MARGIN = sf::Vector2f(15, 15);
        const sf::Vector2f FLAG_SIZE = sf::Vector2f(40, 20);
        const sf::Vector2f FLAG_ORIGIN = sf::Vector2f { 800 - FLAG_MARGIN.x - FLAG_SIZE.x, FLAG_MARGIN.y };

        {
            sf::RectangleShape cflag(FLAG_SIZE);
            cflag.setPosition(FLAG_ORIGIN);

            if (flags.circuit >= 0 && flags.circuit < cflag_color_map.size()) {
                cflag.setFillColor(cflag_color_map[flags.circuit]);
            } else {
                cflag.setFillColor(sf::Color::Magenta);
            }
            
            window.draw(cflag);
        }

        {
            sf::RectangleShape vflag(FLAG_SIZE);
            vflag.setPosition(FLAG_ORIGIN + sf::Vector2f(0, FLAG_MARGIN.y + FLAG_SIZE.y));

            if (flags.vehicle >= 0 && flags.vehicle < vflag_color_map.size()) {
                vflag.setFillColor(vflag_color_map[flags.vehicle]);
            } else {
                vflag.setFillColor(sf::Color::Magenta);
            }

            window.draw(vflag);
        }

        window.display();
    }

    // Wait until the communicator is shut down. Here, this occurs when the user presses Ctrl+C.
    communicator->waitForShutdown();
    runner.join();

    // std::cout << hello.message << std::endl;
}