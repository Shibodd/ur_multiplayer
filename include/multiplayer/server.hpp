#pragma once

#include <Ice/Identity.h>
#include <iostream>

#include <Participant.h>
#include <Server.h>
#include <Lobby.h>

namespace ur {
namespace multiplayer {

class ServerServant : public idl::Server
{
public:
    void hrUpdate(idl::Duration clientTxStamp, idl::Duration clientMinRxDelay, idl::VehicleState state, const Ice::Current& current) override;
    void requestCircuitFlag(idl::FlagId flag, const Ice::Current& current) override;
    void requestVehicleFlag(idl::ParticipantId participantId, idl::FlagId flag, const Ice::Current& current) override;
};

struct LobbyServant : public idl::Lobby {
    void join(std::string version, std::optional<idl::ParticipantPrx> participant, const Ice::Current& current) override;

private:
    struct {
        idl::Duration update_interval_ms;
        idl::Duration race_control_interval_ms;
    } m_hello_data;

    struct OneParticipant {
        idl::ParticipantPrx participant;
        idl::ServerPrx server;
        unsigned short id;

        OneParticipant(const idl::ParticipantPrx& participant, const idl::ServerPrx& server, unsigned short id)
            : participant(participant), server(server), id(id) {}
    };
    std::unordered_map<std::string, OneParticipant> m_participants;
    unsigned short m_next_id = 0;
};

} // namespace multiplayer
} // namespace ur