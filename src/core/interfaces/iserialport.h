#pragma once

#include <cstdint>

namespace AqualinkAutomate::Interfaces
{

    class ISerialPort
    {
    public:
        ISerialPort() = default;
        virtual ~ISerialPort() = default;

    public:
        using DataType = uint8_t;
    };

}
// namespace AqualinkAutomate::Interfaces
