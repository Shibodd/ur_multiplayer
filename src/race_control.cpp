#include "FlagId.h"
#include "RaceControlClient.h"
#include "multiplayer/lobby.hpp"
#include "multiplayer/participant_id.hpp"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <thread>

#include <multiplayer/race_control.hpp>

namespace ur {
namespace multiplayer {

void race_control_run(std::weak_ptr<RaceControlServant> servant, std::weak_ptr<LobbyServant> lobby, std::chrono::milliseconds interval, std::atomic_bool& stop_token) {
    auto timeout = std::chrono::milliseconds { 2500 };
    auto next = std::chrono::steady_clock::now();

    while (not stop_token) {
        std::this_thread::sleep_until(next);
        auto t = std::chrono::steady_clock::now();
        next = t + interval;

        auto rc_ptr = servant.lock();
        auto lobby_ptr = lobby.lock();
        if (not rc_ptr or not lobby_ptr) {
            return;
        }

        idl::Vehicles vehicles;
        {
            Participants participants(*lobby_ptr);
            vehicles.reserve(participants.size());
            for (auto part : participants) {
                if (auto ptr = part.second.lock()) {
                    idl::Vehicle vehicle;
                    vehicle.participantId.value = static_cast<idl_participant_id_t>(ptr->participant_id());
                    vehicle.flagId.value = ptr->vehicle_flag();
                    vehicles.push_back(vehicle);
                }
            }
        }

        idl::FlagId cflag { rc_ptr->circuit_flag(t).first };

        RaceControlClients clients(*rc_ptr);
        auto it = clients.begin();
        auto end = clients.end();

        for (; it != end; ++it) {
            if (it->last_update - t > timeout) {
                it = clients.erase(it);
            }

            it->proxy->update(cflag, vehicles);
        }
    }
}


RaceControlClients::RaceControlClients(RaceControlServant& race_control)
    : m_clients(race_control.m_clients), m_lock(race_control.m_clients_mtx)
{}
std::vector<RaceControlClient>::iterator RaceControlClients::begin() const { return m_clients.begin(); }
std::vector<RaceControlClient>::iterator RaceControlClients::end() const { return m_clients.end(); }
std::vector<RaceControlClient>::iterator RaceControlClients::erase(std::vector<RaceControlClient>::iterator it) { return m_clients.erase(it); }
std::size_t RaceControlClients::size() const { return m_clients.size(); }

RaceControlClient::RaceControlClient(idl::RaceControlClientPrx proxy)
    : proxy(proxy), last_update(std::chrono::steady_clock::now())
{}

RaceControlServant::RaceControlServant(
    std::weak_ptr<LobbyServant> lobby,
    std::chrono::milliseconds debounce_interval,
    std::chrono::milliseconds circuit_delay)
        : m_lobby(lobby), m_debounce_interval(debounce_interval), m_circuit_delay(circuit_delay), m_scheduled_flags(8)
{}

void RaceControlServant::registerClient(std::optional<idl::RaceControlClientPrx> client, const Ice::Current& current) {
    if (client.has_value()) {
        std::scoped_lock lock(m_clients_mtx);

        auto it = std::find_if(m_clients.begin(), m_clients.end(), [id = client->ice_getIdentity()](const RaceControlClient& c) {
            return c.proxy->ice_getIdentity() == id;
        });

        // double up both as keepalive and actual registration
        if (it == m_clients.end()) {
            m_clients.push_back(*client);
        } else {
            it->last_update = std::chrono::steady_clock::now();
        }
    }
}

std::pair<flag_id_t, std::optional<ScheduledFlag>> RaceControlServant::circuit_flag(std::chrono::steady_clock::time_point t) {
    std::scoped_lock lock(m_scheduled_flags_mtx);

    auto begin = m_scheduled_flags.begin();
    auto next_it = std::find_if(begin, m_scheduled_flags.end(), [t](const ScheduledFlag& flag) {
        return flag.activation_stamp > t;
    });
    
    // keep next_it-1 : end (i.e. the very last flag <= t and all flags > t)
    // next_it is thus the candidate scheduled flag
    // std::prev(next_it) is the candidate current flag

    flag_id_t current_flag = 0;
    if (next_it != begin) {
        auto cur_it = std::prev(next_it);
        current_flag = cur_it->id;

        cur_it = m_scheduled_flags.erase(begin, cur_it);
        next_it = std::next(cur_it);
    }

    if (next_it != m_scheduled_flags.end()) {
        return { current_flag, *next_it };
    }
    
    return { current_flag, std::nullopt };
}

void RaceControlServant::requestCircuitFlag(idl::FlagId flag, const Ice::Current& current) {
    auto request_t = std::chrono::steady_clock::now();
    auto schedule_t = request_t + m_circuit_delay;

    std::scoped_lock lock(m_scheduled_flags_mtx);

    if (m_scheduled_flags.full()) {
        return;
    }

    bool add = true;
    if (not m_scheduled_flags.empty()) {
        auto& previousFlag = m_scheduled_flags.back();
        add = previousFlag.id != flag.value && (schedule_t - previousFlag.activation_stamp) > m_debounce_interval;
    }

    if (add) {
        m_scheduled_flags.push_back({ flag.value, schedule_t });
    } else {
        std::cout << "Discard flag request" << std::endl;
    }

    for (auto flag : m_scheduled_flags) {
        std::cout << flag.activation_stamp.time_since_epoch().count() << ", " << int(flag.id) << std::endl;
    }
}

void RaceControlServant::requestVehicleFlag(idl::ParticipantId participantId, idl::FlagId flag, const Ice::Current& current) {
    std::weak_ptr<ServerServant> participantPtr;

    // Get the participant ptr
    if (auto ptr = m_lobby.lock()) {
        Participants participants(*ptr);
        auto it = participants.find(static_cast<participant_id_t>(participantId.value));
        if (it != participants.end()) {
            participantPtr = it->second;
        }
    }

    // Set the vehicle flag
    if (auto ptr = participantPtr.lock()) {
        ptr->set_vehicle_flag(flag.value);
    }
}

} // namespace multiplayer
} // namespace ur