#include <charconv>
#include <format>
#include <limits>

#include <boost/regex.hpp>
#include <magic_enum.hpp>

#include "jandy/utility/string_manipulation.h"
#include "jandy/utility/string_conversion/chemistry.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Utility
{

	Chemistry::Chemistry() noexcept :
		m_ORP(0),
		m_PH(0.0f),
		m_ErrorOccurred(std::nullopt)
	{
	}

	Chemistry::Chemistry(const std::string& chemistry_string) noexcept :
		m_ORP(0),
		m_PH(0.0f),
		m_ErrorOccurred(std::nullopt)
	{
		ConvertStringToChemistry(TrimWhitespace(chemistry_string));
	}

	Chemistry::Chemistry(const Chemistry& other) noexcept :
		m_ORP(other.m_ORP),
		m_PH(other.m_PH),
		m_ErrorOccurred(other.m_ErrorOccurred)
	{
	}

	Chemistry::Chemistry(Chemistry&& other) noexcept :
		m_ORP(std::move(other.m_ORP)),
		m_PH(std::move(other.m_PH)),
		m_ErrorOccurred(std::move(other.m_ErrorOccurred))
	{
	}

	Chemistry& Chemistry::operator=(const Chemistry& other) noexcept
	{
		if (this != &other)
		{
			m_ORP = other.m_ORP;
			m_PH = other.m_PH;
			m_ErrorOccurred = other.m_ErrorOccurred;
		}

		return *this;
	}

	Chemistry& Chemistry::operator=(Chemistry&& other) noexcept
	{
		if (this != &other)
		{
			m_ORP = std::move(other.m_ORP);
			m_PH = std::move(other.m_PH);
			m_ErrorOccurred = std::move(other.m_ErrorOccurred);
		}

		return *this;
	}

	Chemistry& Chemistry::operator=(const std::string& chemistry_string) noexcept
	{
		ConvertStringToChemistry(TrimWhitespace(chemistry_string));
		return *this;
	}

	tl::expected<uint16_t, boost::system::error_code> Chemistry::ORP() const noexcept
	{
		if (m_ErrorOccurred.has_value())
		{
			return tl::unexpected<boost::system::error_code>(make_error_code(m_ErrorOccurred.value()));
		}

		return m_ORP;
	}

	tl::expected<float_t, boost::system::error_code> Chemistry::PH() const noexcept
	{
		if (m_ErrorOccurred.has_value())
		{
			return tl::unexpected<boost::system::error_code>(make_error_code(m_ErrorOccurred.value()));
		}

		return m_PH;
	}

	void Chemistry::ConvertStringToChemistry(const std::string& chemistry_string) noexcept
	{
		const auto [orp, ph] = ValidateAndExtractData(chemistry_string);
		if (orp && ph)
		{
			uint16_t converted_orp;
			float_t converted_ph;

			if (auto [_, ec] = std::from_chars((*orp).data(), (*orp).data() + (*orp).size(), converted_orp); std::errc() != ec)
			{
				LogDebug(Channel::Devices, std::format("Failed to convert ORP; could not convert to number: error -> {}", magic_enum::enum_name(ec)));
				m_ErrorOccurred = ErrorCodes::StringConversion_ErrorCodes::MalformedInput;
			}
			else if (auto [_, ec] = std::from_chars((*ph).data(), (*ph).data() + (*ph).size(), converted_ph); std::errc() != ec)
			{
				LogDebug(Channel::Devices, std::format("Failed to convert pH; could not convert to number: error -> {}", magic_enum::enum_name(ec)));
				m_ErrorOccurred = ErrorCodes::StringConversion_ErrorCodes::MalformedInput;
			}
			else
			{
				m_ORP = static_cast<decltype(m_ORP)>(converted_orp);
				m_PH = static_cast<decltype(m_PH)>(converted_ph);
			}
		}
		else
		{
			LogDebug(Channel::Devices, std::format("Failed to convert ORP and/or pH; malformed input -> {}", chemistry_string));
			m_ErrorOccurred = ErrorCodes::StringConversion_ErrorCodes::MalformedInput;
		}
	}

	std::tuple<std::optional<std::string>, std::optional<std::string>> Chemistry::ValidateAndExtractData(const std::string& chemistry_string) noexcept
	{
		if (MINIMUM_STRING_LENGTH > chemistry_string.size() || MAXIMUM_STRING_LENGTH < chemistry_string.size())
		{
			return { std::nullopt, std::nullopt };
		}

		boost::regex re("(ORP\\/([2-9][0-9]{2}|[2-9][0-9]{1}))\\s+(PH\\/([6-7]\\.[0-9]|8\\.([0-1][0-9]|2)))");
		boost::smatch match;

		if (boost::regex_match(chemistry_string, match, re))
		{
			// NOTE: This regex will capture the following groups:
			//
			//    Group 1 -> ORP/###
			//    Group 2 -> ###
			//    Group 3 -> PH/#.#
			//    Group 4 -> #.#
			//

			return { match[2].str(), match[4].str() };
		}
		else
		{
			return { std::nullopt, std::nullopt };
		}
	}

}
// namespace AqualinkAutomate::Utility
