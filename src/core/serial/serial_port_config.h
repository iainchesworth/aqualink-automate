#pragma once

#include <cstdint>
#include <compare>

#include "serial/serial_port_enums.h"

namespace AqualinkAutomate::Serial
{

    struct SerialPortConfig
    {
        uint32_t baud_rate = 9600;
        uint8_t character_size = 8;
        Parity parity = Parity::None;
        StopBits stop_bits = StopBits::One;
        FlowControl flow_control = FlowControl::None;

        auto operator<=>(const SerialPortConfig&) const = default;

        [[nodiscard]] static constexpr SerialPortConfig Default();
        [[nodiscard]] static constexpr SerialPortConfig JandyDefault() noexcept
        {
            return SerialPortConfig{
                .baud_rate = 9600,
                .character_size = 8,
                .parity = Parity::None,
                .stop_bits = StopBits::One,
                .flow_control = FlowControl::None
            };
        }

        [[nodiscard]] constexpr bool IsValid() const noexcept;
    };

}
// namespace AqualinkAutomate::Serial
