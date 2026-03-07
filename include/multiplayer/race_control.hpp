#pragma once

#include <Participant.h>
#include <RaceControl.h>

#include <multiplayer/participant_id.hpp>

namespace ur {
namespace multiplayer {

class RaceControlServant : public idl::RaceControl
{
public:
    void requestCircuitFlag(idl::FlagId flag, const Ice::Current& current) override;
    void requestVehicleFlag(idl::ParticipantId participantId, idl::FlagId flag, const Ice::Current& current) override;
};

} // namespace multiplayer
} // namespace ur