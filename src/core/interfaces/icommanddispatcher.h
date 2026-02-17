#pragma once

#include <cstdint>
#include <string>

#include <boost/uuid/uuid.hpp>

namespace AqualinkAutomate::Interfaces
{

	class ICommandDispatcher
	{
	public:
		enum class CommandResult
		{
			Success,
			DeviceNotFound,
			NoSerialAdapter,
			UnknownEquipmentType,
			InvalidValue
		};

		enum class DeviceAction
		{
			On,
			Off,
			Toggle
		};

		virtual ~ICommandDispatcher() = default;

		virtual CommandResult ToggleByUuid(const boost::uuids::uuid& uuid) = 0;
		virtual CommandResult ToggleByLabel(const std::string& label) = 0;
		virtual CommandResult CommandByUuid(const boost::uuids::uuid& uuid, DeviceAction action) = 0;
		virtual CommandResult CommandByLabel(const std::string& label, DeviceAction action) = 0;
		virtual CommandResult SetPoolSetpoint(uint8_t temperature) = 0;
		virtual CommandResult SetSpaSetpoint(uint8_t temperature) = 0;
	};

}
// namespace AqualinkAutomate::Interfaces
