#pragma once

#include <Participant.h>

namespace ur {
namespace multiplayer {

struct Participant : public idl::Participant {
    void hello(idl::Hello hello, const Ice::Current& current);
    void hrUpdate(idl::Timestamp serverTxStamp, idl::Opponents opponents, const Ice::Current& current) override;
    void updateFlags(idl::ScheduledFlags flags, const Ice::Current& current) override;
};

} // namespace multiplayer
} // namespace ur