#pragma once

#include "Timestamp.ice"
#include "Duration.ice"
#include "ParticipantId.ice"
#include "FlagId.ice"
#include "VehicleState.ice"

module ur::multiplayer
{
    struct Opponent
    {
        ParticipantId participantId;
        VehicleState vehicleState;
    }
    sequence<Opponent> Opponents;

    struct ServerToClientHRUpdate
    {
        Timestamp serverTxStamp;
        Opponents opponents;
    }

    struct ScheduledFlag
    {
        FlagId id;
        Timestamp activationStamp;
    }
    sequence<ScheduledFlag> ScheduledFlags;

    interface Participant
    {
        void hrUpdate(ServerToClientHRUpdate update);
        void updateFlags(ScheduledFlags flags);
    }
}