#pragma once

#include "ParticipantId.ice"
#include "FlagId.ice"

module ur::multiplayer::idl
{
    struct Vehicle
    {
        ParticipantId participantId;
        FlagId flagId;
    }
    sequence<Vehicle> Vehicles;

    interface RaceControlClient
    {
        void update(FlagId circuitFlag, Vehicles vehicles);
    }
}