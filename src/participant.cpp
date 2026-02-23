#include <multiplayer/participant.hpp>
#include <iostream>

namespace ur {
namespace multiplayer {

void Participant::hrUpdate(idl::Timestamp serverTxStamp, idl::Opponents opponents, const Ice::Current& current) {
    std::cout << "hr update" << std::endl;
}

void Participant::updateFlags(idl::ScheduledFlags flags, const Ice::Current& current) {

}

void Participant::hello(idl::Hello hello, const Ice::Current& current) {
    std::cout << hello.message << std::endl;
}

} // namespace multiplayer
} // namespace ur