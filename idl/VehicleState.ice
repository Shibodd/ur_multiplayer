#pragma once

#include "Vector2f.ice"
#include "Timestamp.ice"

module ur::multiplayer::idl
{
    struct VehicleState
    {
        Timestamp stamp;
        
        Vector2f position;
        Vector2f velocity;
        float yaw;
        float yawRate;
    }
}