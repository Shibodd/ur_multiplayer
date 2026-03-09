#include "multiplayer/server.hpp"
#include <climits>
#include <iostream>
#include <algorithm>
#include <memory>

#include <multiplayer/participant_id.hpp>
#include <multiplayer/lobby.hpp>

namespace ur {
namespace multiplayer {

Participants::Participants(LobbyServant& lobby) 
    : m_servants(lobby.m_active_servants), m_lock(lobby.m_active_servants_mtx) {}

ActiveServants::iterator Participants::find(participant_id_t id) const { return m_servants.find(id); }
ActiveServants::iterator Participants::begin() const { return m_servants.begin(); };
ActiveServants::iterator Participants::end() const { return m_servants.end(); }
ActiveServants::iterator Participants::erase(ActiveServants::iterator it) {
    return m_servants.erase(it);
}

std::size_t Participants::size() const { return m_servants.size(); }


void LobbyServant::join(std::string version, std::optional<idl::ParticipantPrx> participant, const Ice::Current& current) {
    if (not participant.has_value()) {
        std::cout << "Join request with empty participant discarded." << std::endl;
        return;
    }

    idl::Hello hello;
    if (version != m_version) {
        std::cout << "Join request with wrong version discarded." << std::endl;

        hello.message = "Incompatible version. Server is running " + m_version;
        participant->hello(hello);
        return;
    }

    participant_id_t id = m_next_id.fetch_add(1);
    Ice::Identity server_identity { std::to_string(id), "server" };

    auto server_servant = std::make_shared<ServerServant>(*participant, id);
    auto server_proxy = current.adapter->add(server_servant, server_identity);

    {
        std::scoped_lock lock(m_active_servants_mtx);
        m_active_servants.emplace(id, server_servant);
    }

    hello.participantId.value = static_cast<idl_participant_id_t>(id);
    hello.message = "Success";
    hello.server = Ice::uncheckedCast<idl::ServerPrx>(server_proxy)->ice_datagram();
    participant->hello(hello);

    std::cout << "A new participant has joined." << std::endl;
}

} // namespace multiplayer
} // namespace ur