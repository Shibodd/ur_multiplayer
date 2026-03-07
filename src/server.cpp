#include "Lobby.h"
#include "ParticipantId.h"
#include "Server.h"
#include <Ice/Initialize.h>
#include <chrono>
#include <iostream>

#include <memory>
#include <multiplayer/server.hpp>

#include <Ice/ProxyFunctions.h>
#include <stdexcept>
#include <thread>
#include <tuple>
#include <utility>

namespace ur {
namespace multiplayer {

ServerStartupToken::ServerStartupToken(ServerServant& servant) : m_servant(servant) { }
void ServerStartupToken::release() {
    m_servant.start();
    m_should_cleanup = false;
}
ServerStartupToken::~ServerStartupToken() {
    if (m_should_cleanup) {
        m_servant.cleanup();
    }
}

ServerServant::ServerServant(idl::ParticipantPrx participant, participant_id_t id, std::chrono::milliseconds update_interval, std::chrono::milliseconds update_timeout)
    : m_id(id), m_proxy(participant), m_interval(update_interval), m_timeout(update_timeout) {
}

ServerServant::~ServerServant() {
    std::cout << "destroy" << std::endl;
    m_stop_token = true;
    if (m_thread.joinable()) {
        m_thread.join();
    }
    cleanup();
}

void ServerServant::start() {
    bool val = false;
    if (m_started.compare_exchange_strong(val, true)) {
        m_last_update = last_update_clock_t::now();
        m_thread = std::thread([&]() { run(); });
    } else {
        throw std::runtime_error("Already started.");
    }
}

void ServerServant::cleanup() {
    bool val = false;
    if (m_cleanup_callback && m_cleaned_up.compare_exchange_strong(val, true)) {
        m_cleaned_up = true;
        m_cleanup_callback();
    }
}

ServerStartupToken ServerServant::set_cleanup_callback(std::function<void()> callback) {
    m_cleanup_callback = callback;
    return ServerStartupToken { *this };
}

void ServerServant::run() {
    auto next = std::chrono::steady_clock::now();
    while (not m_stop_token) {
        std::this_thread::sleep_until(next);
        auto t = std::chrono::steady_clock::now();
        next = t + m_interval;

        if (t - m_last_update > m_timeout) {
            std::cout << "Participant " << m_id << " timed out." << std::endl;
            break;
        }

        // Retrieve states from other servants
        // Prepare message
        // Send data
    }

    cleanup();
}

void ServerServant::hrUpdate(idl::Duration clientTxStamp, idl::Duration clientMinRxDelay, idl::VehicleState state, const Ice::Current& current) {
    m_last_update = last_update_clock_t::now();
}

} // namespace multiplayer
} // namespace ur