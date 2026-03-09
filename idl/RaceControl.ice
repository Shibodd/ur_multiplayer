#pragma once

#include "FlagId.ice"
#include "ParticipantId.ice"
#include "RaceControlClient.ice"

module ur::multiplayer::idl
{
    interface RaceControl
    {
        void registerClient(RaceControlClient* client);
        void requestCircuitFlag(FlagId flag);
        void requestVehicleFlag(ParticipantId participantId, FlagId flag);
    }
}