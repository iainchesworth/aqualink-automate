#pragma once

#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <tuple>

#include <boost/system/error_code.hpp>

#include "jandy/errors/string_conversion_errors.h"

namespace AqualinkAutomate::Utility
{

	class EquipmentStatus
	{
		static const uint8_t MAXIMUM_STRING_LENGTH = 16;
		static const uint8_t MINIMUM_STRING_LENGTH = 4;

	public:
		enum class Statuses
		{
			On,
			Off, 
			Enabled,
			Pending,
			Unknown
		};

	public:
		EquipmentStatus() noexcept;
		EquipmentStatus(const std::string& equipment_status_string) noexcept;
		EquipmentStatus(const EquipmentStatus& other) noexcept;
		EquipmentStatus(EquipmentStatus&& other) noexcept;

	public:
		EquipmentStatus& operator=(const EquipmentStatus& other) noexcept;
		EquipmentStatus& operator=(EquipmentStatus&& other) noexcept;
		EquipmentStatus& operator=(const std::string& equipment_status_string) noexcept;

	public:
		std::expected<std::tuple<std::string, Statuses>, boost::system::error_code> operator()() const noexcept;
		std::expected<std::string, boost::system::error_code> Name() const noexcept;
		std::expected<Statuses, boost::system::error_code> Status() const noexcept;


	private:
		void ConvertStringToStatus(const std::string& equipment_status_string) noexcept;
		std::tuple<std::optional<std::string>, std::optional<std::string>> ValidateAndExtractData(const std::string& equipment_status_string) noexcept;

	private:
		std::string m_Name;
		Statuses m_Status;

	private:
		std::optional<ErrorCodes::StringConversion_ErrorCodes> m_ErrorOccurred;
	};

}
// namespace AqualinkAutomate::Utility
