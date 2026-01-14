#pragma once

#include "ParticipantId.ice"
#include "Duration.ice"

module ur::multiplayer
{
    struct ServerMetadata
    {
        long version;
        Duration updateIntervalMs;
        Duration raceControlIntervalMs;
        ParticipantId participantId;
    }
}