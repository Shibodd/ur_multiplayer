#pragma once


#include <atomic>
#include <chrono>
#include <iostream>

#include <Ice/Identity.h>

#include <Participant.h>
#include <Server.h>
#include <Lobby.h>

#include <multiplayer/participant_id.hpp>

namespace ur {
namespace multiplayer {

class ServerServant;

struct ServerStartupToken {
    ServerStartupToken(ServerServant& servant);
    ~ServerStartupToken();

    ServerStartupToken(const ServerStartupToken&) = delete;
    ServerStartupToken& operator=(const ServerStartupToken&) = delete;
    ServerStartupToken(ServerStartupToken&&) = delete;
    ServerStartupToken& operator=(ServerStartupToken&&) = delete;
    
    void release();
private:
    bool m_should_cleanup = true;
    ServerServant& m_servant;
};

class ServerServant : public idl::Server
{
public:
    ServerServant(
        idl::ParticipantPrx participant,
        participant_id_t id,
        std::chrono::milliseconds update_interval,
        std::chrono::milliseconds timeout
    );

    ~ServerServant();

    void hrUpdate(idl::Duration clientTxStamp, idl::Duration clientMinRxDelay, idl::VehicleState state, const Ice::Current& current) override;

    void run();
    ServerStartupToken set_cleanup_callback(std::function<void()> callback);
    void start();

    participant_id_t participant_id() const { return m_id; }

    friend struct ServerStartupToken;
private:
    using last_update_clock_t = std::chrono::steady_clock;

    void cleanup();

    idl::ParticipantPrx m_proxy;
    participant_id_t m_id;
    last_update_clock_t::time_point m_last_update;

    std::atomic_bool m_started = false;
    std::chrono::milliseconds m_interval;
    std::chrono::milliseconds m_timeout;
    
    std::atomic_bool m_cleaned_up = false;
    std::function<void()> m_cleanup_callback;

    std::thread m_thread;
    std::atomic_bool m_stop_token = false;
};

} // namespace multiplayer
} // namespace ur