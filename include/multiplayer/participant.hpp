#pragma once

#include "Server.h"
#include "clock_sync/clock_offset_calculator.hpp"
#include <Participant.h>

#include <multiplayer/participant_id.hpp>

namespace ur {
namespace multiplayer {

struct Participant : public idl::Participant {
    Participant();

    void hello(idl::Hello hello, const Ice::Current& current) override;
    void hrUpdate(
        idl::Timestamp serverTxStamp,
        idl::Duration serverMinRxDelay,
        idl::Opponents opponents,
        idl::FlagId vehicleFlag,
        idl::FlagId circuitFlag,
        idl::ScheduledFlag scheduledCircuitFlag,
        const Ice::Current& current) override;
    
    bool has_joined() const { return m_data.has_value(); }
    auto server_proxy() const { return m_data->proxy; };
    auto clock_offset() {
        std::scoped_lock lock(m_offset_calc_mtx);
        return m_offset_calculator.get_tx_delay(clock_sync::ClockOffsetCalculator::Clock::now());
    }

private:
    std::mutex m_offset_calc_mtx;
    clock_sync::ClockOffsetCalculator m_offset_calculator;

    struct Data {
        idl::ServerPrx proxy;
        participant_id_t id;
    };
    std::optional<Data> m_data;
};

} // namespace multiplayer
} // namespace ur