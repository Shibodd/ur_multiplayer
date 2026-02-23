#include "Lobby.h"
#include "ParticipantId.h"
#include "Server.h"
#include <Ice/Initialize.h>
#include <iostream>

#include <multiplayer/server.hpp>

#include <Ice/ProxyFunctions.h>
#include <tuple>
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
    if (not participant.has_value()) {
        std::cout << "Empty join request discarded" << std::endl;
        return;
    }

    idl::Hello hello;
    hello.updateIntervalMs = m_hello_data.update_interval_ms;
    hello.raceControlIntervalMs = m_hello_data.race_control_interval_ms;

    std::string participant_identity = Ice::identityToString(participant->ice_getIdentity());

    {
        auto it = m_participants.find(participant_identity);
        if (it != m_participants.end()) {
            std::cout << "Duplicate join request by " << participant_identity << std::endl;

            hello.message = "Already connected";
            hello.participantId = idl::ParticipantId { static_cast<short>(it->second.id) };
            hello.server = it->second.server;
            participant->hello(hello);
            return;
        }
    }

    if (version != "") {
        std::cout << "Join request with wrong version by " << participant_identity << std::endl;

        hello.message = "Incompatible version!";
        participant->hello(hello);
        return;
    }

    auto server = Ice::uncheckedCast<idl::ServerPrx>(current.adapter->addWithUUID(std::make_shared<ServerServant>())).ice_datagram();
    
    auto [it,_] = m_participants.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(participant_identity),
        std::forward_as_tuple(*participant, server, m_next_id++)
    );

    hello.message = "Connected";
    hello.participantId = idl::ParticipantId { static_cast<short>(it->second.id) };
    hello.server = server;
    participant->hello(hello);

    std::cout << "Completed join request by " << participant_identity << std::endl;
}

} // namespace multiplayer
} // namespace ur