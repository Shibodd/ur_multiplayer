#pragma once

#include "Server.h"
#include <Participant.h>

namespace ur {
namespace multiplayer {

struct Participant : public idl::Participant {
    void hello(idl::Hello hello, const Ice::Current& current);
    void hrUpdate(idl::Timestamp serverTxStamp, idl::Opponents opponents, const Ice::Current& current) override;
    void updateFlags(idl::ScheduledFlags flags, const Ice::Current& current) override;
    
    auto server() const { return m_server; };

private:
    std::optional<idl::ServerPrx> m_server;
};

} // namespace multiplayer
} // namespace ur