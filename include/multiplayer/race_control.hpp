#pragma once

#include <chrono>
#include <memory>

#include <boost/circular_buffer.hpp>

#include <RaceControlClient.h>
#include <Participant.h>
#include <RaceControl.h>

#include <multiplayer/lobby.hpp>
#include <multiplayer/participant_id.hpp>

namespace ur {
namespace multiplayer {

struct RaceControlServant;

void race_control_run(std::weak_ptr<RaceControlServant> servant, std::weak_ptr<LobbyServant> lobby, std::chrono::milliseconds interval, std::atomic_bool& stop_token);

struct RaceControlClient {
    std::chrono::steady_clock::time_point last_update;
    idl::RaceControlClientPrx proxy;
    RaceControlClient(idl::RaceControlClientPrx proxy);
};

struct RaceControlClients {
    RaceControlClients(RaceControlServant& race_control);

    std::vector<RaceControlClient>::iterator begin() const;
    std::vector<RaceControlClient>::iterator end() const;
    std::vector<RaceControlClient>::iterator erase(std::vector<RaceControlClient>::iterator it);
    std::size_t size() const;
    
private:
    std::vector<RaceControlClient>& m_clients;
    std::unique_lock<std::mutex> m_lock;
};

using flag_id_t = decltype(idl::FlagId::value);

struct ScheduledFlag {
    flag_id_t id;
    std::chrono::steady_clock::time_point activation_stamp;

    ScheduledFlag(flag_id_t id, std::chrono::steady_clock::time_point stamp)
        : id(id), activation_stamp(stamp) {}
};

class RaceControlServant : public idl::RaceControl
{
public: 

    RaceControlServant(std::weak_ptr<LobbyServant> lobby, std::chrono::milliseconds debounce_interval, std::chrono::milliseconds circuit_delay);

    void registerClient(std::optional<idl::RaceControlClientPrx> client, const Ice::Current& current) override;
    void requestCircuitFlag(idl::FlagId flag, const Ice::Current& current) override;
    void requestVehicleFlag(idl::ParticipantId participantId, idl::FlagId flag, const Ice::Current& current) override;

    std::pair<flag_id_t, std::optional<ScheduledFlag>> circuit_flag(std::chrono::steady_clock::time_point t);

    friend struct RaceControlClients;

private:
    std::chrono::milliseconds m_debounce_interval;
    std::chrono::milliseconds m_circuit_delay;

    std::mutex m_scheduled_flags_mtx;
    boost::circular_buffer<ScheduledFlag> m_scheduled_flags;

    std::weak_ptr<LobbyServant> m_lobby;

    std::mutex m_clients_mtx;
    std::vector<RaceControlClient> m_clients;
};

} // namespace multiplayer
} // namespace ur