#include <format>

#include <re2/re2.h>

#include "jandy/utility/string_manipulation.h"
#include "jandy/utility/string_conversion/auxillary_state.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Utility
{
	const std::string AuxillaryState::REGEX_PATTERN{ R"((.{1,13})\s+(ON|OFF|ENA|\*\*\*))" };
	const boost::regex AuxillaryState::REGEX_PARSER{ REGEX_PATTERN };

	AuxillaryState::AuxillaryState() noexcept :
		m_Label(),
		m_State(Kernel::AuxillaryStatuses::Unknown),
		m_ErrorOccurred(std::nullopt)
	{
	}

	AuxillaryState::AuxillaryState(const std::string& auxillary_status_string) noexcept :
		m_Label(),
		m_State(Kernel::AuxillaryStatuses::Unknown),
		m_ErrorOccurred(std::nullopt)
	{
		ConvertStringToStatus(auxillary_status_string);
	}

	AuxillaryState::AuxillaryState(const AuxillaryState& other) noexcept :
		m_Label(other.m_Label),
		m_State(other.m_State),
		m_ErrorOccurred(other.m_ErrorOccurred)
	{
	}

	AuxillaryState::AuxillaryState(AuxillaryState&& other) noexcept :
		m_Label(std::move(other.m_Label)),
		m_State(std::move(other.m_State)),
		m_ErrorOccurred(std::move(other.m_ErrorOccurred))
	{
	}

	AuxillaryState& AuxillaryState::operator=(const AuxillaryState& other) noexcept
	{
		if (this != &other)
		{
			m_Label = other.m_Label;
			m_State = other.m_State;
			m_ErrorOccurred = other.m_ErrorOccurred;
		}

		return *this;
	}

	AuxillaryState& AuxillaryState::operator=(AuxillaryState&& other) noexcept
	{
		if (this != &other)
		{
			m_Label = std::move(other.m_Label);
			m_State = std::move(other.m_State);
			m_ErrorOccurred = std::move(other.m_ErrorOccurred);
		}

		return *this;
	}

	AuxillaryState& AuxillaryState::operator=(const std::string& auxillary_status_string) noexcept
	{
		ConvertStringToStatus(auxillary_status_string);
		return *this;
	}

	tl::expected<std::string, boost::system::error_code> AuxillaryState::Label() const noexcept
	{
		if (m_ErrorOccurred.has_value())
		{
			return tl::unexpected<boost::system::error_code>(make_error_code(m_ErrorOccurred.value()));
		}

		return m_Label;
	}

	tl::expected<Kernel::AuxillaryStatuses, boost::system::error_code> AuxillaryState::State() const noexcept
	{
		if (m_ErrorOccurred.has_value())
		{
			return tl::unexpected<boost::system::error_code>(make_error_code(m_ErrorOccurred.value()));
		}

		return m_State;
	}

	void AuxillaryState::ConvertStringToStatus(const std::string& auxillary_status_string) noexcept
	{
		const auto auxillary_status_data = ValidateAndExtractData(auxillary_status_string);

		const auto name = std::get<0>(auxillary_status_data);
		const auto status = std::get<1>(auxillary_status_data);

		if (name && status)
		{
			m_Label = TrimWhitespace(name.value());

			if ("ON" == status.value())
			{
				m_State = Kernel::AuxillaryStatuses::On;
			}
			else if ("OFF" == status.value())
			{
				m_State = Kernel::AuxillaryStatuses::Off;
			}
			else if ("ENA" == status.value())
			{
				m_State = Kernel::AuxillaryStatuses::Enabled;
			}
			else if ("***" == status.value())
			{
				m_State = Kernel::AuxillaryStatuses::Pending;
			}
			else
			{
				LogDebug(Channel::Devices, std::format("Failed to convert equipment status; could not convert state value -> {}", status.value()));
				m_ErrorOccurred = ErrorCodes::StringConversion_ErrorCodes::MalformedInput;
			}
		}
		else
		{
			LogDebug(Channel::Devices, std::format("Failed to convert equipment status; malformed input -> {}", auxillary_status_string));
			m_ErrorOccurred = ErrorCodes::StringConversion_ErrorCodes::MalformedInput;
		}
	}

	std::tuple<std::optional<std::string>, std::optional<std::string>> AuxillaryState::ValidateAndExtractData(const std::string& auxillary_status_string) noexcept
	{
		boost::smatch match_results;

		if (MINIMUM_STRING_LENGTH > auxillary_status_string.size() || MAXIMUM_STRING_LENGTH < auxillary_status_string.size())
		{
			// Invalid string length...do nothing.
		}
		else if (!boost::regex_search(auxillary_status_string, match_results, REGEX_PARSER))
		{
			// Invalid pattern match...do nothing.	
		}
		else if (2 > match_results.size())
		{
			// Insufficent resultset to pull groups from.
		}
		else
		{
			return std::make_tuple<>(
				std::optional<std::string>(match_results[1]),
				std::optional<std::string>(match_results[2])
			);
		}

		return std::make_tuple(std::nullopt, std::nullopt);
	}

}
// namespace AqualinkAutomate::Utility
