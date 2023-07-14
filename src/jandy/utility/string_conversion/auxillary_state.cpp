#include <format>

#include <re2/re2.h>

#include "jandy/utility/string_manipulation.h"
#include "jandy/utility/string_conversion/auxillary_state.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Utility
{

	AuxillaryState::AuxillaryState() noexcept :
		m_Label(),
		m_State(Kernel::AuxillaryStates::Unknown),
		m_ErrorOccurred(std::nullopt)
	{
	}

	AuxillaryState::AuxillaryState(const std::string& auxillary_status_string) noexcept :
		m_Label(),
		m_State(Kernel::AuxillaryStates::Unknown),
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

	tl::expected<Kernel::AuxillaryStates, boost::system::error_code> AuxillaryState::State() const noexcept
	{
		if (m_ErrorOccurred.has_value())
		{
			return tl::unexpected<boost::system::error_code>(make_error_code(m_ErrorOccurred.value()));
		}

		return m_State;
	}

	void AuxillaryState::ConvertStringToStatus(const std::string& auxillary_status_string) noexcept
	{
		const auto [name, status] = ValidateAndExtractData(auxillary_status_string);
		if (name && status)
		{
			m_Label = TrimWhitespace(name.value());

			if ("ON" == status.value())
			{
				m_State = Kernel::AuxillaryStates::On;
			}
			else if ("OFF" == status.value())
			{
				m_State = Kernel::AuxillaryStates::Off;
			}
			else if ("ENA" == status.value())
			{
				m_State = Kernel::AuxillaryStates::Enabled;
			}
			else if ("***" == status.value())
			{
				m_State = Kernel::AuxillaryStates::Pending;
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
		if (MINIMUM_STRING_LENGTH > auxillary_status_string.size() || MAXIMUM_STRING_LENGTH < auxillary_status_string.size())
		{
			return { std::nullopt, std::nullopt };
		}

		re2::RE2 re(R"((.{1,13})\s+(ON|OFF|ENA|\*\*\*))");
		std::string match1, match2;

		if (re2::RE2::FullMatch(auxillary_status_string, re, &match1, &match2))
		{
			return { match1, match2 };
		}
		else
		{
			return { std::nullopt, std::nullopt };
		}
	}

}
// namespace AqualinkAutomate::Utility
