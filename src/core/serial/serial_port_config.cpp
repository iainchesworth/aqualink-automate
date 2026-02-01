#include "serial/serial_port_config.h"

namespace AqualinkAutomate::Serial
{

    constexpr SerialPortConfig SerialPortConfig::Default()
    {
        return SerialPortConfig{};
    }

    constexpr bool SerialPortConfig::IsValid() const noexcept
    {
        return baud_rate > 0 && character_size >= 5 && character_size <= 8;
    }

}
// namespace AqualinkAutomate::Serial
