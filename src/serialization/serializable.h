#pragma once

#include <cstddef>
#include <span>

namespace AqualinkAutomate::Serialization
{

    class Serializable
    {
    public:
        virtual void Serialize(std::span<const std::byte>& message_bytes) const = 0;
        virtual void Deserialize(const std::span<const std::byte>& message_bytes) = 0;
    };

} 
// namespace AqualinkAutomate::Serialization
