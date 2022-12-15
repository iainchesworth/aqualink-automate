#pragma once

#include <iostream>

namespace AqualinkAutomate::Serialization
{

    class Serializable
    {
    public:
        virtual void Serialize(std::ostream& stream) const = 0;
        virtual void Deserialize(std::istream& stream) = 0;
    };

} 
// namespace AqualinkAutomate::Serialization
