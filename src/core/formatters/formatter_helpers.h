#pragma once

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <format>
#include <iterator>
#include <ostream>
#include <ranges>
#include <string>
#include <string_view>

namespace AqualinkAutomate::Formatters
{

	// Compile-time string usable as a non-type template parameter (NTTP). Lets QuantityFormatter
	// carry its unit suffix (" ppm", "mV", "V", ...) directly in the type, so a single template
	// replaces the otherwise-identical per-unit formatters.
	template<std::size_t N>
	struct FixedString
	{
		char m_Data[N]{};

		consteval FixedString(const char (&literal)[N])
		{
			std::copy_n(literal, N, m_Data);
		}

		[[nodiscard]] constexpr std::string_view View() const noexcept
		{
			// N includes the trailing NUL written by the literal copy; exclude it from the view.
			return std::string_view(m_Data, N - 1);
		}
	};

	// A range of bytes (uint8_t elements) that can be iterated and indexed/iterated for a hex dump.
	// Covers std::array<uint8_t, N>, std::span<uint8_t>, std::span<const uint8_t> and std::vector<uint8_t>.
	template<typename R>
	concept ByteRange =
		std::ranges::input_range<R> &&
		std::same_as<std::remove_cvref_t<std::ranges::range_value_t<R>>, std::uint8_t>;

	// Append a space-separated lower-case hex dump ("0xAB 0xCD ...") of the supplied bytes to the
	// format output iterator. Shared by the array/span/vector and circular-buffer formatters so the
	// per-byte loop lives in exactly one place. No leading or trailing separator is emitted.
	template<std::input_iterator It, typename OutputIt>
	OutputIt FormatHexBytes(It first, It last, OutputIt out)
	{
		bool first_byte = true;

		for (It it = first; it != last; ++it)
		{
			if (!first_byte)
			{
				*out++ = ' ';
			}

			first_byte = false;
			out = std::format_to(out, "0x{:02x}", static_cast<std::uint8_t>(*it));
		}

		return out;
	}

	template<ByteRange R, typename OutputIt>
	OutputIt FormatHexBytes(const R& bytes, OutputIt out)
	{
		return FormatHexBytes(std::ranges::begin(bytes), std::ranges::end(bytes), out);
	}

	// Single ADL/stream adapter: render any std::formatter-formattable value into an ostream via
	// std::format, replacing the per-.cpp "os << std::format(\"{}\", obj); return os;" boilerplate.
	template<typename T>
	std::ostream& WriteFormatted(std::ostream& os, const T& value)
	{
		os << std::format("{}", value);
		return os;
	}

	// Shared formatter for a boost::units quantity rendered as "<value><SUFFIX>". The numeric value is
	// produced by std::formatter<double> (so the {:.Nf} spec still applies) and SUFFIX is appended
	// afterwards. Replaces the otherwise byte-identical ppm / millivolt / volt formatters and fixes
	// the discarded-iterator bug (the base format() result is now threaded through).
	template<typename Quantity, FixedString SUFFIX>
	struct QuantityFormatter
	{
		std::formatter<double> m_Base;

		constexpr auto parse(std::format_parse_context& ctx)
		{
			return m_Base.parse(ctx);
		}

		template<typename FormatContext>
		auto format(const Quantity& value, FormatContext& ctx) const
		{
			ctx.advance_to(m_Base.format(value.value(), ctx));
			return std::ranges::copy(SUFFIX.View(), ctx.out()).out;
		}
	};

}
// namespace AqualinkAutomate::Formatters
