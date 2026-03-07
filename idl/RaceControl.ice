#pragma once

#include "FlagId.ice"
#include "ParticipantId.ice"

module ur::multiplayer::idl
{
    interface RaceControl
    {
        void requestCircuitFlag(FlagId flag);
        void requestVehicleFlag(ParticipantId participantId, FlagId flag);
    }
}