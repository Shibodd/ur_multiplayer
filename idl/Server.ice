#pragma once

#include "ServerMetadata.ice"
#include "VehicleState.ice"
#include "Duration.ice"
#include "ParticipantId.ice"
#include "FlagId.ice"

module ur::multiplayer
{
    struct ClientToServerHRUpdate
    {
        VehicleState state;
        Duration clientTxStamp;
        Duration clientMinRxDelay;
    }

    interface Server
    {
        ServerMetadata getServerMetadata();
        void hrUpdate(ClientToServerHRUpdate update);
        void requestCircuitFlag(FlagId flag);
        void requestVehicleFlag(ParticipantId participantId, FlagId flag);
    }
}