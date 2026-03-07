#pragma once


#include <chrono>
#include <iostream>

#include <Ice/Identity.h>

#include <Participant.h>
#include <Server.h>
#include <Lobby.h>

#include <multiplayer/participant_id.hpp>

namespace ur {
namespace multiplayer {

class ServerServant : public idl::Server
{
public:
    ServerServant(idl::ParticipantPrx participant, participant_id_t id);

    void hrUpdate(idl::Duration clientTxStamp, idl::Duration clientMinRxDelay, idl::VehicleState state, const Ice::Current& current) override;

private:
    using last_update_clock_t = std::chrono::steady_clock;

    idl::ParticipantPrx m_proxy;
    participant_id_t m_id;
    last_update_clock_t::time_point m_last_update;
};

} // namespace multiplayer
} // namespace ur