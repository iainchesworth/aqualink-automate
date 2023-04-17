#pragma once

#include <cstdint>

namespace AqualinkAutomate::Interfaces
{

    class ISerialPort
    {
    public:
        ISerialPort();
        virtual ~ISerialPort();

    public:
        using DataType = uint8_t;
    };

}
// namespace AqualinkAutomate::Interfaces
