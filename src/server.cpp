#include "FlagId.h"
#include "Participant.h"
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <tuple>
#include <utility>
#include <memory>

#include <multiplayer/server.hpp>

#include <clock_sync/clock_offset_calculator.hpp>
#include <Ice/ProxyFunctions.h>

namespace ur {
namespace multiplayer {

ServerServant::ServerServant(idl::ParticipantPrx participant, participant_id_t id)
    : m_id(id), m_proxy(participant),
      m_rx_delay_window(clock_sync::ClockOffsetCalculator::Filter::Params{ 8, std::chrono::seconds{10}, 256}),
      m_last_update(last_update_clock_t::now()) {
}

idl::VehicleState ServerServant::get_state() {
    std::scoped_lock lock(m_state_mtx);
    return m_state;
}

void ServerServant::hrUpdate(idl::Duration clientTxStamp, idl::VehicleState state, const Ice::Current& current) {
    auto rx_stamp = clock_sync::ClockOffsetCalculator::Clock::now();
    auto tx_stamp = clock_sync::ClockOffsetCalculator::Timepoint { std::chrono::nanoseconds{ clientTxStamp.value } };

    m_last_update = last_update_clock_t::now();
    
    {
        std::scoped_lock lock(m_state_mtx);
        m_state = state;
    }

    {
        std::scoped_lock lock(m_rx_delay_window_mtx);
        m_rx_delay_window.push(decltype(m_rx_delay_window)::Measurement{ rx_stamp, rx_stamp - tx_stamp });
    }
}

} // namespace multiplayer
} // namespace ur