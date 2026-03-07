#include <iostream>

#include <multiplayer/participant.hpp>

namespace ur {
namespace multiplayer {

void Participant::hrUpdate(idl::Timestamp serverTxStamp, idl::Opponents opponents, const Ice::Current& current) {
    std::cout << "hr update" << std::endl;
}

void Participant::updateFlags(idl::ScheduledFlags flags, const Ice::Current& current) {
    std::cout << "flags update" << std::endl;
}

void Participant::hello(idl::Hello hello, const Ice::Current& current) {
    if (not hello.server.has_value()) {
        std::cout << "Server declined join request! " << hello.message << std::endl;
        m_data.reset();
        return;
    }

    m_data = Data {
        .proxy = *hello.server,
        .id = static_cast<participant_id_t>(hello.participantId.value),
        .updateInterval = std::chrono::milliseconds(hello.updateIntervalMs.value),
        .raceControlInterval = std::chrono::milliseconds(hello.raceControlIntervalMs.value)
    };
}

} // namespace multiplayer
} // namespace ur