#pragma once

#include <memory>
#include <multiplayer/participant_id.hpp>
#include <multiplayer/server.hpp>

#include <boost/circular_buffer.hpp>
#include <Lobby.h>

namespace ur {
namespace multiplayer {

struct LobbyServant : public idl::Lobby, public std::enable_shared_from_this<LobbyServant> {
    void join(std::string version, std::optional<idl::ParticipantPrx> participant, const Ice::Current& current) override;

    using ActiveServantSeq = std::vector<std::weak_ptr<ServerServant>>;

private:
    std::chrono::milliseconds m_update_interval = std::chrono::milliseconds(20);
    std::chrono::milliseconds m_race_control_interval = std::chrono::milliseconds(100);
    std::chrono::milliseconds m_update_timeout = std::chrono::milliseconds(2500);
    
    std::string m_version = "";
    
    std::atomic<participant_id_t> m_next_id = 0;

    std::mutex m_active_servants_mtx;
    std::unordered_map<participant_id_t, std::weak_ptr<ServerServant>> m_active_servants;
};

} // namespace multiplayer
} // namespace ur