#pragma once

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
			UnknownEquipmentType
		};

		virtual ~ICommandDispatcher() = default;

		virtual CommandResult ToggleByUuid(const boost::uuids::uuid& uuid) = 0;
		virtual CommandResult ToggleByLabel(const std::string& label) = 0;
	};

}
// namespace AqualinkAutomate::Interfaces
