#include <RaceControl.h>
#include <Ice/Endpoint.h>
#include <Ice/Identity.h>
#include <climits>
#include <iostream>

#include <memory>
#include <multiplayer/participant_id.hpp>

#include <multiplayer/server.hpp>
#include <multiplayer/lobby.hpp>

namespace ur {
namespace multiplayer {

void LobbyServant::join(std::string version, std::optional<idl::ParticipantPrx> participant, const Ice::Current& current) {
    if (not participant.has_value()) {
        std::cout << "Join request with empty participant discarded." << std::endl;
        return;
    }

    idl::Hello hello;
    hello.raceControlIntervalMs = m_hello_data.race_control_interval_ms;
    hello.updateIntervalMs = m_hello_data.update_interval_ms;

    if (version != m_version) {
        std::cout << "Join request with wrong version discarded." << std::endl;

        hello.message = "Incompatible version. Server is running " + m_version;
        participant->hello(hello);
        return;
    }

    participant_id_t id = ++m_next_id;
    Ice::Identity server_identity { std::to_string(id), "server" };

    auto server_servant = std::make_shared<ServerServant>(*participant, id);
    m_active_servants.push_back(server_servant);

    hello.participantId.value = static_cast<idl_participant_id_t>(id);
    hello.message = "Success";
    hello.server = Ice::uncheckedCast<idl::ServerPrx>(current.adapter->add(server_servant, server_identity)).ice_datagram();
    participant->hello(hello);

    std::cout << "A new participant has joined." << std::endl;
}

} // namespace multiplayer
} // namespace ur