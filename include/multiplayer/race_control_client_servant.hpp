#pragma once

#include <RaceControlClient.h>

namespace ur {
namespace multiplayer {


struct RaceControlClientServant : public idl::RaceControlClient {
    void update(idl::FlagId circuitFlag, idl::Vehicles vehicles, const Ice::Current& current) override;

    struct Data {
        idl::FlagId circuit_flag;
        idl::Vehicles vehicles;
    };

    Data data();
private:
    std::mutex m_mutex;
    Data m_data;
};

} // namespace multiplayer
} // namespace ur