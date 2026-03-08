#pragma once

#include "Duration.ice"
#include "VehicleState.ice"
#include "FlagId.ice"
#include "ParticipantId.ice"

module ur::multiplayer::idl
{
    interface Server
    {
        void hrUpdate(Duration clientTxStamp, VehicleState state);
    }
}