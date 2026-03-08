#pragma once

#include "Vector2f.ice"
#include "Timestamp.ice"

module ur::multiplayer::idl
{
    struct VehicleState
    {
        Timestamp stamp;
        
        Vector2f position;
        float vx;
        float yaw;

        float steering;
        float throttle;
    }
}