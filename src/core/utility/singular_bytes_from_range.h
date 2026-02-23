#pragma once

#include <memory>
#include <span>

namespace AqualinkAutomate::Utility
{

    template<class T>
    std::span<T, 1> SingularSpan(T& t)
    {
        return std::span<T, 1>{ std::addressof(t), 1 };
    }

    template<class T>
    auto SingularBytes(T& t)
    {
        return std::as_bytes(SingularSpan(t));
    }

}
// namespace AqualinkAutomate::Utility
