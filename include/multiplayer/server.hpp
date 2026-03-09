#pragma once

#include "FlagId.h"
#include "Participant.h"
#include <atomic>
#include <chrono>
#include <iostream>

#include <clock_sync/clock_offset_calculator.hpp>

#include <multiplayer/lobby.hpp>
#include <multiplayer/participant_id.hpp>

namespace ur {
namespace multiplayer {

class LobbyServant;

class ServerServant : public idl::Server
{
public:
    using last_update_clock_t = std::chrono::steady_clock;

    ServerServant(
        idl::ParticipantPrx participant,
        participant_id_t id
    );

    ~ServerServant() { std::cout << "destroy " << m_id << std::endl; }

    idl::VehicleState get_state();

    void hrUpdate(idl::Duration clientTxStamp, idl::VehicleState state, const Ice::Current& current) override;
    idl::ParticipantPrx participant_proxy() const { return m_proxy; }

    last_update_clock_t::time_point last_update() const { return m_last_update; }
    auto min_rx_delay() {
        std::scoped_lock lock(m_rx_delay_window_mtx);
        return m_rx_delay_window.minimum();
    }

    participant_id_t participant_id() const { return m_id; }

    void set_vehicle_flag(decltype(idl::FlagId::value) flag) { m_flag = flag; }
    auto vehicle_flag() const { return m_flag.load(); }

private:
    std::atomic<decltype(idl::FlagId::value)> m_flag;

    std::mutex m_state_mtx;
    idl::VehicleState m_state;

    std::mutex m_rx_delay_window_mtx;
    clock_sync::utils::WindowMinimum<
        clock_sync::ClockOffsetCalculator::Timepoint,
        clock_sync::ClockOffsetCalculator::Duration> m_rx_delay_window;

    idl::ParticipantPrx m_proxy;
    participant_id_t m_id;
    last_update_clock_t::time_point m_last_update;
};

} // namespace multiplayer
} // namespace ur