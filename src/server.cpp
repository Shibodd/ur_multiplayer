#include "Lobby.h"
#include "ParticipantId.h"
#include "Server.h"
#include <Ice/Initialize.h>
#include <iostream>

#include <multiplayer/server.hpp>

#include <Ice/ProxyFunctions.h>
#include <utility>

namespace ur {
namespace multiplayer {

void ServerServant::hrUpdate(idl::Duration clientTxStamp, idl::Duration clientMinRxDelay, idl::VehicleState state, const Ice::Current& current) {
    std::cout << "update" << std::endl;
}
void ServerServant::requestCircuitFlag(idl::FlagId flag, const Ice::Current& current) {
    std::cout << "cflag" << std::endl;
}
void ServerServant::requestVehicleFlag(idl::ParticipantId participantId, idl::FlagId flag, const Ice::Current& current) {
    std::cout << "vflag" << std::endl;
}

void LobbyServant::join(std::string version, std::optional<idl::ParticipantPrx> participant, const Ice::Current& current) {
    std::cout << "join" << std::endl;

    if (not participant.has_value()) {
        return;
    }

    idl::Hello hello;
    hello.updateIntervalMs = m_hello_data.update_interval_ms;
    hello.raceControlIntervalMs = m_hello_data.race_control_interval_ms;

    std::string participant_identity = Ice::identityToString(participant->ice_getIdentity());

    {
        auto it = m_participants.find(participant_identity);
        if (it != m_participants.end()) {
            hello.message = "Already connected";
            hello.participantId = idl::ParticipantId { static_cast<short>(it->second.id) };
            hello.server = it->second.server;
            participant->ice_datagram()->hello(hello);
            return;
        }
    }

    if (version != "") {
        hello.message = "Incompatible version!";
        participant->ice_datagram()->hello(hello);
        return;
    }

    auto server = Ice::uncheckedCast<idl::ServerPrx>(current.adapter->addWithUUID(std::make_shared<ServerServant>()));
    auto [it,_] = m_participants.emplace(
        std::piecewise_construct,
        std::make_tuple(participant_identity),
        std::make_tuple(*participant, server, m_next_id++)
    );

    hello.message = "Connected";
    hello.participantId = idl::ParticipantId { static_cast<short>(it->second.id) };
    hello.server = server;
    participant->ice_datagram()->hello(hello);
}

} // namespace multiplayer
} // namespace ur