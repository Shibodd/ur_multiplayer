#include "Lobby.h"
#include "ParticipantId.h"
#include "Server.h"
#include <Ice/Initialize.h>
#include <chrono>
#include <iostream>

#include <multiplayer/server.hpp>

#include <Ice/ProxyFunctions.h>
#include <tuple>
#include <utility>

namespace ur {
namespace multiplayer {

ServerServant::ServerServant(idl::ParticipantPrx participant, participant_id_t id)
    : m_id(id), m_proxy(participant), m_last_update(last_update_clock_t::now()) {
}

void ServerServant::hrUpdate(idl::Duration clientTxStamp, idl::Duration clientMinRxDelay, idl::VehicleState state, const Ice::Current& current) {
    m_last_update = last_update_clock_t::now();
}

} // namespace multiplayer
} // namespace ur