#include <chrono>
#include <iostream>

#include <clock_sync/clock_offset_calculator.hpp>

#include <multiplayer/participant.hpp>

namespace ur {
namespace multiplayer {

Participant::Participant() : m_offset_calculator(clock_sync::ClockOffsetCalculator::Filter::Params{ 8, std::chrono::seconds{10}, 256}) {

}

void Participant::hrUpdate(
        idl::Timestamp serverTxStamp,
        idl::Duration serverMinRxDelay,
        idl::Opponents opponents,
        idl::FlagId vehicleFlag,
        idl::FlagId circuitFlag,
        idl::ScheduledFlag scheduledCircuitFlag, 
        const Ice::Current& current) {
    auto rx_stamp = std::chrono::steady_clock::now();
    auto tx_stamp = clock_sync::ClockOffsetCalculator::Timepoint { std::chrono::nanoseconds { serverTxStamp.nanoseconds } };

    std::chrono::nanoseconds clock_offset;
    {
        std::scoped_lock lock(m_offset_calc_mtx);
        m_offset_calculator.on_our_rx_delay(rx_stamp, clock_sync::ClockOffsetCalculator::Timepoint { std::chrono::nanoseconds { serverTxStamp.nanoseconds } });
        if (std::chrono::nanoseconds{ serverMinRxDelay.value } != std::chrono::nanoseconds::max()) {
            m_offset_calculator.on_their_rx_delay(rx_stamp, std::chrono::nanoseconds{ serverMinRxDelay.value });
        }
        clock_offset = m_offset_calculator.clock_offset(rx_stamp).value_or(std::chrono::nanoseconds(0));
    }

    for (auto& opponent : opponents) {
        opponent.vehicleState.stamp.nanoseconds -= clock_offset.count();
    }

    {
        std::scoped_lock lock(m_opponents_mtx);
        m_opponents = opponents;
    }

    {
        std::scoped_lock lock(m_flags_mtx);
        m_flags.circuit = circuitFlag.value;
        m_flags.vehicle = vehicleFlag.value;

        if (scheduledCircuitFlag.activationStamp.nanoseconds > 0) {
            std::chrono::steady_clock::time_point cact_t { std::chrono::nanoseconds { scheduledCircuitFlag.activationStamp.nanoseconds } };

            m_flags.scheduled_circuit.emplace();
            m_flags.scheduled_circuit->activation_t = cact_t - clock_offset;
            m_flags.scheduled_circuit->id = scheduledCircuitFlag.id.value;
            // update flags to make sure scheduled flag wasn't already triggered.
            update_scheduled_flag(rx_stamp);
        }
    }
}

void Participant::hello(idl::Hello hello, const Ice::Current& current) {
    if (not hello.server.has_value()) {
        std::cout << "Server declined join request! " << hello.message << std::endl;
        m_data.reset();
        return;
    }

    m_data = Data {
        .proxy = *hello.server,
        .id = static_cast<participant_id_t>(hello.participantId.value)
    };
}

void Participant::update_scheduled_flag(std::chrono::steady_clock::time_point t) {
    if (m_flags.scheduled_circuit.has_value()) {
        if (t > m_flags.scheduled_circuit->activation_t) {
            m_flags.circuit = m_flags.scheduled_circuit->id;
            m_flags.scheduled_circuit.reset();
        }
    }
}

Flags Participant::flags(std::chrono::steady_clock::time_point t) {
    std::scoped_lock lock(m_flags_mtx);

    update_scheduled_flag(t);

    return Flags {
        .circuit = m_flags.circuit,
        .vehicle = m_flags.vehicle
    };
}

} // namespace multiplayer
} // namespace ur