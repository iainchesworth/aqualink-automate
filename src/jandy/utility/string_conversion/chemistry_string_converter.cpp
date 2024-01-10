#include <charconv>
#include <format>
#include <limits>

#include <boost/cstdfloat.hpp>
#include <magic_enum.hpp>

#include "jandy/utility/string_manipulation.h"
#include "jandy/utility/string_conversion/chemistry_string_converter.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Utility
{
	const std::string ChemistryStringConverter::REGEX_PATTERN{ "(ORP\\/([2-9][0-9]{2}|[2-9][0-9]{1}))\\s+(PH\\/([6-7]\\.[0-9]|8\\.([0-1][0-9]|2)))" };
	const boost::regex ChemistryStringConverter::REGEX_PARSER{ REGEX_PATTERN };

	ChemistryStringConverter::ChemistryStringConverter() noexcept :
		m_ORP(0),
		m_PH(0.0f),
		m_ErrorOccurred(std::nullopt)
	{
	}

	ChemistryStringConverter::ChemistryStringConverter(const std::string& chemistry_string) noexcept :
		m_ORP(0),
		m_PH(0.0f),
		m_ErrorOccurred(std::nullopt)
	{
		ConvertStringToChemistry(TrimWhitespace(chemistry_string));
	}

	ChemistryStringConverter::ChemistryStringConverter(const ChemistryStringConverter& other) noexcept :
		m_ORP(other.m_ORP),
		m_PH(other.m_PH),
		m_ErrorOccurred(other.m_ErrorOccurred)
	{
	}

	ChemistryStringConverter::ChemistryStringConverter(ChemistryStringConverter&& other) noexcept :
		m_ORP(std::move(other.m_ORP)),
		m_PH(std::move(other.m_PH)),
		m_ErrorOccurred(std::move(other.m_ErrorOccurred))
	{
	}

	ChemistryStringConverter& ChemistryStringConverter::operator=(const ChemistryStringConverter& other) noexcept
	{
		if (this != &other)
		{
			m_ORP = other.m_ORP;
			m_PH = other.m_PH;
			m_ErrorOccurred = other.m_ErrorOccurred;
		}

		return *this;
	}

	ChemistryStringConverter& ChemistryStringConverter::operator=(ChemistryStringConverter&& other) noexcept
	{
		if (this != &other)
		{
			m_ORP = std::move(other.m_ORP);
			m_PH = std::move(other.m_PH);
			m_ErrorOccurred = std::move(other.m_ErrorOccurred);
		}

		return *this;
	}

	ChemistryStringConverter& ChemistryStringConverter::operator=(const std::string& chemistry_string) noexcept
	{
		ConvertStringToChemistry(TrimWhitespace(chemistry_string));
		return *this;
	}

	tl::expected<Kernel::ORP, boost::system::error_code> ChemistryStringConverter::ORP() const noexcept
	{
		if (m_ErrorOccurred.has_value())
		{
			return tl::unexpected<boost::system::error_code>(make_error_code(m_ErrorOccurred.value()));
		}

		return m_ORP;
	}

	tl::expected<Kernel::pH, boost::system::error_code> ChemistryStringConverter::PH() const noexcept
	{
		if (m_ErrorOccurred.has_value())
		{
			return tl::unexpected<boost::system::error_code>(make_error_code(m_ErrorOccurred.value()));
		}

		return m_PH;
	}

	void ChemistryStringConverter::ConvertStringToChemistry(const std::string& chemistry_string) noexcept
	{
		const auto chemistry_data = ValidateAndExtractData(chemistry_string);

		const auto orp = std::get<0>(chemistry_data);
		const auto ph = std::get<1>(chemistry_data);

		if (orp && ph)
		{
			boost::float64_t converted_orp;
			boost::float32_t converted_ph;

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
				m_ORP = converted_orp;
				m_PH = converted_ph;
			}
		}
		else
		{
			LogDebug(Channel::Devices, std::format("Failed to convert ORP and/or pH; malformed input -> {}", chemistry_string));
			m_ErrorOccurred = ErrorCodes::StringConversion_ErrorCodes::MalformedInput;
		}
	}

	std::tuple<std::optional<std::string>, std::optional<std::string>> ChemistryStringConverter::ValidateAndExtractData(const std::string& chemistry_string) noexcept
	{
		boost::smatch match_results;

		if (MINIMUM_STRING_LENGTH > chemistry_string.size() || MAXIMUM_STRING_LENGTH < chemistry_string.size())
		{
			// Invalid string length...do nothing.
		}
		else if (!boost::regex_search(chemistry_string, match_results, REGEX_PARSER))
		{
			// Invalid pattern match...do nothing.
		}
		else if (4 > match_results.size())
		{
			// Insufficent resultset to pull groups from.
		}
		else
		{
			// NOTE: This regex will capture the following groups:
			//
			//    Group 1 -> ORP/###
			//    Group 2 -> ###
			//    Group 3 -> PH/#.#
			//    Group 4 -> #.#
			//

			return std::make_tuple<>(
				std::optional<std::string>(match_results[2]),
				std::optional<std::string>(match_results[4])
			);
		}

		return std::make_tuple(std::nullopt, std::nullopt);
	}

}
// namespace AqualinkAutomate::Utility
