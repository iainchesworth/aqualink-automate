#include "kernel/equipment_versions.h"

#include <algorithm>

namespace AqualinkAutomate::Kernel
{

	void EquipmentVersions::Set(const std::string& label, const std::string& value)
	{
		auto it = std::find_if(Fields.begin(), Fields.end(),
			[&](const EquipmentVersionField& f) { return f.label == label; });

		if (it != Fields.end())
		{
			it->value = value;
		}
		else
		{
			Fields.push_back({ label, value });
		}
	}

	std::string EquipmentVersions::Get(const std::string& label) const
	{
		auto it = std::find_if(Fields.begin(), Fields.end(),
			[&](const EquipmentVersionField& f) { return f.label == label; });

		return (it != Fields.end()) ? it->value : std::string{};
	}

	std::string EquipmentVersions::ModelNumber() const
	{
		return Get("Model");
	}

	std::string EquipmentVersions::FirmwareRevision() const
	{
		return Get("Revision");
	}

}
// namespace AqualinkAutomate::Kernel
