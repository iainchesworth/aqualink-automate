#include <boost/regex.hpp>

#include "jandy/utility/string_manipulation.h"
#include "jandy/utility/string_conversion/equipment_status.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Utility
{

	EquipmentStatus::EquipmentStatus() noexcept :
		m_Name(),
		m_Status(Statuses::Unknown),
		m_ErrorOccurred(std::nullopt)
	{
	}

	EquipmentStatus::EquipmentStatus(const std::string& equipment_status_string) noexcept :
		m_Name(),
		m_Status(Statuses::Unknown),
		m_ErrorOccurred(std::nullopt)
	{
		ConvertStringToStatus(equipment_status_string);
	}

	EquipmentStatus::EquipmentStatus(const EquipmentStatus& other) noexcept :
		m_Name(other.m_Name),
		m_Status(other.m_Status),
		m_ErrorOccurred(other.m_ErrorOccurred)
	{
	}

	EquipmentStatus::EquipmentStatus(EquipmentStatus&& other) noexcept :
		m_Name(std::move(other.m_Name)),
		m_Status(std::move(other.m_Status)),
		m_ErrorOccurred(std::move(other.m_ErrorOccurred))
	{
	}

	EquipmentStatus& EquipmentStatus::operator=(const EquipmentStatus& other) noexcept
	{
		if (this != &other)
		{
			m_Name = other.m_Name;
			m_Status = other.m_Status;
			m_ErrorOccurred = other.m_ErrorOccurred;
		}

		return *this;
	}

	EquipmentStatus& EquipmentStatus::operator=(EquipmentStatus&& other) noexcept
	{
		if (this != &other)
		{
			m_Name = std::move(other.m_Name);
			m_Status = std::move(other.m_Status);
			m_ErrorOccurred = std::move(other.m_ErrorOccurred);
		}

		return *this;
	}

	EquipmentStatus& EquipmentStatus::operator=(const std::string& equipment_status_string) noexcept
	{
		ConvertStringToStatus(equipment_status_string);
		return *this;
	}

	std::expected<std::tuple<std::string, EquipmentStatus::Statuses>, boost::system::error_code> EquipmentStatus::operator()() const noexcept
	{
		if (m_ErrorOccurred.has_value())
		{
			return std::unexpected<boost::system::error_code>(make_error_code(m_ErrorOccurred.value()));
		}

		return std::make_tuple(m_Name, m_Status);
	}

	std::expected<std::string, boost::system::error_code> EquipmentStatus::Name() const noexcept
	{
		if (m_ErrorOccurred.has_value())
		{
			return std::unexpected<boost::system::error_code>(make_error_code(m_ErrorOccurred.value()));
		}

		return m_Name;
	}

	std::expected<EquipmentStatus::Statuses, boost::system::error_code> EquipmentStatus::Status() const noexcept
	{
		if (m_ErrorOccurred.has_value())
		{
			return std::unexpected<boost::system::error_code>(make_error_code(m_ErrorOccurred.value()));
		}

		return m_Status;
	}

	void EquipmentStatus::ConvertStringToStatus(const std::string& equipment_status_string) noexcept
	{
		const auto [name, status] = ValidateAndExtractData(equipment_status_string);
		if (name && status)
		{
			m_Name = TrimWhitespace(name.value());

			if ("ON" == status.value())
			{
				m_Status = Statuses::On;
			}
			else if ("OFF" == status.value())
			{
				m_Status = Statuses::Off;
			}
			else if ("ENA" == status.value())
			{
				m_Status = Statuses::Enabled;
			}
			else if ("***" == status.value())
			{
				m_Status = Statuses::Pending;
			}
			else
			{
				LogDebug(Channel::Devices, std::format("Failed to convert equipment status; could not convert status value -> {}", status.value()));
				m_ErrorOccurred = ErrorCodes::StringConversion_ErrorCodes::MalformedInput;
			}
		}
		else
		{
			LogDebug(Channel::Devices, std::format("Failed to convert equipment status; malformed input -> {}", equipment_status_string));
			m_ErrorOccurred = ErrorCodes::StringConversion_ErrorCodes::MalformedInput;
		}
	}

	std::tuple<std::optional<std::string>, std::optional<std::string>> EquipmentStatus::ValidateAndExtractData(const std::string& equipment_status_string) noexcept
	{
		if (MINIMUM_STRING_LENGTH > equipment_status_string.size() || MAXIMUM_STRING_LENGTH < equipment_status_string.size())
		{
			return { std::nullopt, std::nullopt };
		}

		boost::regex re(R"((.{1,13})\s+(ON|OFF|ENA|\*\*\*))");
		boost::smatch match;

		if (boost::regex_match(equipment_status_string, match, re))
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
