#pragma once

#include "Participant.ice"

module ur::multiplayer::idl
{
    interface Lobby
    {
        void join(string version, Participant* participant);
    }
}