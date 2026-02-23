#pragma once

#include <cstdint>
#include <type_traits>

namespace AqualinkAutomate::Utility
{

    constexpr char digits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    template<std::intmax_t N, int base, typename char_type, std::enable_if_t<(base > 1 && base < sizeof(digits)), int> = 0>
    class ToString_T {

        constexpr static auto buflen() noexcept 
        {
            unsigned int len = N > 0 ? 1 : 2;
            for (auto n = N; n; len++, n /= base);
            return len;
        }

        char_type buf[buflen()] = {};

    public:
        constexpr ToString_T() noexcept
        {
            auto ptr = end();
            *--ptr = '\0';

            if (N != 0) 
            {
                for (auto n = N; n; n /= base)
                {
                    *--ptr = digits[(N < 0 ? -1 : 1) * (n % base)];
                }
                
                if (N < 0)
                {
                    *--ptr = '-';
                }
            }
            else 
            {
                buf[0] = '0';
            }
        }

        constexpr operator char_type* () noexcept { return buf; }
        constexpr operator const char_type* () const noexcept { return buf; }

        constexpr auto size() const noexcept { return sizeof(buf) / sizeof(buf[0]); }

        constexpr auto data() noexcept { return buf; }
        constexpr const auto data() const noexcept { return buf; }
        constexpr auto& operator[](unsigned int i) noexcept { return buf[i]; }
        constexpr const auto& operator[](unsigned int i) const noexcept { return buf[i]; }
        constexpr auto& front() noexcept { return buf[0]; }
        constexpr const auto& front() const noexcept { return buf[0]; }
        constexpr auto& back() noexcept { return buf[size() - 1]; }
        constexpr const auto& back() const noexcept { return buf[size() - 1]; }

        constexpr auto begin() noexcept { return buf; }
        constexpr const auto begin() const noexcept { return buf; }
        constexpr auto end() noexcept { return buf + size(); }
        constexpr const auto end() const noexcept { return buf + size(); }
    };

    // Simplifies use of `ToString_T` from `ToString_T<N>()` to `ToString<N>`.
    template<std::intmax_t N, int base = 10, typename char_type = char>
    constexpr ToString_T<N, base, char_type> ToString;

}
// namespace AqualinkAutomate::Utility
