#include <iostream>

#include <multiplayer/race_control_client_servant.hpp>

namespace ur {
namespace multiplayer {

void RaceControlClientServant::update(idl::FlagId circuitFlag, idl::Vehicles vehicles, const Ice::Current &current) {
    std::cout << "update" << std::endl;
    std::scoped_lock lock(m_mutex);
    m_data.circuit_flag = circuitFlag;
    m_data.vehicles = vehicles;
}

RaceControlClientServant::Data RaceControlClientServant::data() {
    std::scoped_lock lock(m_mutex);
    return m_data;
}

} // namespace multiplayer
} // namespace ur