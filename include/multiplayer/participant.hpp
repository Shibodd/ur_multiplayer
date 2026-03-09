#pragma once

#include "Server.h"
#include "clock_sync/clock_offset_calculator.hpp"
#include <Participant.h>

#include <chrono>
#include <multiplayer/participant_id.hpp>

namespace ur {
namespace multiplayer {

struct Flags {
    decltype(idl::FlagId::value) circuit;
    decltype(idl::FlagId::value) vehicle;
};

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
        return m_offset_calculator.get_tx_delay(std::chrono::steady_clock::now());
    }

    idl::Opponents opponents() {
        std::scoped_lock lock(m_opponents_mtx);
        return m_opponents;
    }

    Flags flags(std::chrono::steady_clock::time_point t);

private:
    std::mutex m_flags_mtx;
    struct {
        struct ScheduledFlag {
            decltype(idl::FlagId::value) id;
            std::chrono::steady_clock::time_point activation_t;
        };

        std::optional<ScheduledFlag> scheduled_circuit;

        decltype(idl::FlagId::value) circuit;
        decltype(idl::FlagId::value) vehicle;
    } m_flags;

    void update_scheduled_flag(std::chrono::steady_clock::time_point t);

    std::mutex m_offset_calc_mtx;
    clock_sync::ClockOffsetCalculator m_offset_calculator;

    std::mutex m_opponents_mtx;
    idl::Opponents m_opponents;

    struct Data {
        idl::ServerPrx proxy;
        participant_id_t id;
    };
    std::optional<Data> m_data;
};

} // namespace multiplayer
} // namespace ur