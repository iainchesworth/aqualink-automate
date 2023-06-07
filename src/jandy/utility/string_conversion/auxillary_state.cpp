#include <boost/regex.hpp>

#include "jandy/utility/string_manipulation.h"
#include "jandy/utility/string_conversion/auxillary_state.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Utility
{

	AuxillaryState::AuxillaryState() noexcept :
		m_Label(),
		m_State(Config::AuxillaryStates::Unknown),
		m_ErrorOccurred(std::nullopt)
	{
	}

	AuxillaryState::AuxillaryState(const std::string& auxillary_status_string) noexcept :
		m_Label(),
		m_State(Config::AuxillaryStates::Unknown),
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

	std::expected<Config::Auxillary, boost::system::error_code> AuxillaryState::operator()() const noexcept
	{
		if (m_ErrorOccurred.has_value())
		{
			return std::unexpected<boost::system::error_code>(make_error_code(m_ErrorOccurred.value()));
		}

		return Config::Auxillary {m_Label, m_State};
	}

	std::expected<std::string, boost::system::error_code> AuxillaryState::Label() const noexcept
	{
		if (m_ErrorOccurred.has_value())
		{
			return std::unexpected<boost::system::error_code>(make_error_code(m_ErrorOccurred.value()));
		}

		return m_Label;
	}

	std::expected<Config::AuxillaryStates, boost::system::error_code> AuxillaryState::State() const noexcept
	{
		if (m_ErrorOccurred.has_value())
		{
			return std::unexpected<boost::system::error_code>(make_error_code(m_ErrorOccurred.value()));
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
				m_State = Config::AuxillaryStates::On;
			}
			else if ("OFF" == status.value())
			{
				m_State = Config::AuxillaryStates::Off;
			}
			else if ("ENA" == status.value())
			{
				m_State = Config::AuxillaryStates::Enabled;
			}
			else if ("***" == status.value())
			{
				m_State = Config::AuxillaryStates::Pending;
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

		boost::regex re(R"((.{1,13})\s+(ON|OFF|ENA|\*\*\*))");
		boost::smatch match;

		if (boost::regex_match(auxillary_status_string, match, re))
		{
			return { match[1].str(), match[2].str() };
		}
		else
		{
			return { std::nullopt, std::nullopt };
		}
	}

}
// namespace AqualinkAutomate::Utility
