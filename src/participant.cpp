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
    auto rx_stamp = clock_sync::ClockOffsetCalculator::Clock::now();
    auto tx_stamp = clock_sync::ClockOffsetCalculator::Timepoint { std::chrono::nanoseconds { serverTxStamp.nanoseconds } };

    std::optional<clock_sync::ClockOffsetCalculator::Duration> clock_offset;
    {
        std::scoped_lock lock(m_offset_calc_mtx);
        m_offset_calculator.on_our_rx_delay(rx_stamp, clock_sync::ClockOffsetCalculator::Timepoint { std::chrono::nanoseconds { serverTxStamp.nanoseconds } });
        if (std::chrono::nanoseconds{ serverMinRxDelay.value } != std::chrono::nanoseconds::max()) {
            m_offset_calculator.on_their_rx_delay(rx_stamp, std::chrono::nanoseconds{ serverMinRxDelay.value });
        }
        clock_offset = m_offset_calculator.clock_offset(rx_stamp);
    }

    for (auto& opponent : opponents) {
        opponent.vehicleState.stamp.nanoseconds -= clock_offset.value_or(std::chrono::nanoseconds(0)).count();
    }

    std::cout << clock_offset.value_or(std::chrono::nanoseconds(0)).count() << std::endl;
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

} // namespace multiplayer
} // namespace ur