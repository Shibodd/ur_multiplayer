#pragma once

#include <memory>
#include <shared_mutex>

#include <Lobby.h>

#include <boost/circular_buffer.hpp>

#include <multiplayer/participant_id.hpp>
#include <multiplayer/server.hpp>

namespace ur {
namespace multiplayer {

class ServerServant;

using ActiveServants = std::unordered_map<participant_id_t, std::weak_ptr<ServerServant>>;

struct LobbyServant;

struct Participants {
    Participants(LobbyServant& lobby);

    ActiveServants::iterator begin() const;
    ActiveServants::iterator end() const;
    ActiveServants::iterator erase(ActiveServants::iterator it);

    std::size_t size() const;
private:
    ActiveServants& m_servants;
    std::unique_lock<std::mutex> m_lock;
};

struct LobbyServant : public idl::Lobby {
    void join(std::string version, std::optional<idl::ParticipantPrx> participant, const Ice::Current& current) override;

    friend class Participants;
private:
    std::chrono::milliseconds m_update_interval = std::chrono::milliseconds(20);
    std::chrono::milliseconds m_race_control_interval = std::chrono::milliseconds(100);
    std::chrono::milliseconds m_update_timeout = std::chrono::milliseconds(2500);
    
    std::string m_version = "";
    
    std::atomic<participant_id_t> m_next_id = 0;

    std::mutex m_active_servants_mtx;
    ActiveServants m_active_servants;
};

} // namespace multiplayer
} // namespace ur