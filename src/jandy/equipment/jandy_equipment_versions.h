#pragma once

#include <string>

#include "jandy/equipment/jandy_equipment_types.h"

namespace AqualinkAutomate::Equipment
{

	struct JandyEquipmentVersions
	{
		std::string ModelNumber;
		std::string FirmwareRevision;
		JandyEquipmentTypes PanelType;
	};

}
// namespace AqualinkAutomate::Equipment