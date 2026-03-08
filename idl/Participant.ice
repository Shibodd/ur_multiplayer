#pragma once

#include "Timestamp.ice"
#include "Duration.ice"
#include "ParticipantId.ice"
#include "FlagId.ice"
#include "VehicleState.ice"
#include "Server.ice"

module ur::multiplayer::idl
{
    struct Opponent
    {
        ParticipantId participantId;
        VehicleState vehicleState;
    }
    sequence<Opponent> Opponents;

    struct ScheduledFlag
    {
        FlagId id;
        Timestamp activationStamp;
    }

    struct Hello
    {
        Server* server;
        string message;
        ParticipantId participantId;
    }

    interface Participant
    {
        void hello(Hello hello);
        void hrUpdate(Timestamp serverTxStamp, Duration serverMinRxDelay, Opponents opponents, FlagId vehicleFlag, FlagId circuitFlag, ScheduledFlag scheduledCircuitFlag);
    }
}