#pragma once

#include <cstdint>
#include <concepts>
#include <vector>

#include "messages/jandy_message.h"

namespace AqualinkAutomate::Concepts
{

    template<typename T>
    concept JandyMessageType = requires(T msg, const std::vector<uint8_t>&bytes) {
        requires std::derived_from<T, Messages::JandyMessage>;
        requires std::is_default_constructible_v<T>;
        requires std::is_nothrow_default_constructible_v<T>;
    
    { msg.DeserializeContents(bytes) } -> std::convertible_to<bool>;

    };

}
// namespace AqualinkAutomate::Concepts
