#include <iostream>

#include <multiplayer/race_control.hpp>

namespace ur {
namespace multiplayer {

void RaceControlServant::requestCircuitFlag(idl::FlagId flag, const Ice::Current& current) {
    std::cout << "cflag" << std::endl;
}
void RaceControlServant::requestVehicleFlag(idl::ParticipantId participantId, idl::FlagId flag, const Ice::Current& current) {
    std::cout << "vflag" << std::endl;
}

} // namespace multiplayer
} // namespace ur