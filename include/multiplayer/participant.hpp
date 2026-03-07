#pragma once

#include "Server.h"
#include <Participant.h>

#include <multiplayer/participant_id.hpp>

namespace ur {
namespace multiplayer {

struct Participant : public idl::Participant {
    void hello(idl::Hello hello, const Ice::Current& current) override;
    void hrUpdate(idl::Timestamp serverTxStamp, idl::Opponents opponents, const Ice::Current& current) override;
    void updateFlags(idl::ScheduledFlags flags, const Ice::Current& current) override;
    
    bool has_joined() const { return m_data.has_value(); }
    auto server_proxy() const { return m_data->proxy; };

private:
    struct Data {
        idl::ServerPrx proxy;
        participant_id_t id;
        std::chrono::nanoseconds updateInterval;
        std::chrono::nanoseconds raceControlInterval;
    };
    std::optional<Data> m_data;
};

} // namespace multiplayer
} // namespace ur