#pragma once

#include <multiplayer/participant_id.hpp>
#include <multiplayer/server.hpp>

#include <boost/circular_buffer.hpp>
#include <Lobby.h>

namespace ur {
namespace multiplayer {

struct LobbyServant : public idl::Lobby {
    void join(std::string version, std::optional<idl::ParticipantPrx> participant, const Ice::Current& current) override;

private:
    struct {
        idl::Duration update_interval_ms;
        idl::Duration race_control_interval_ms;
    } m_hello_data;
    
    participant_id_t m_next_id = 0;
    std::string m_version = "";
    std::vector<std::weak_ptr<ServerServant>> m_active_servants;
};

} // namespace multiplayer
} // namespace ur