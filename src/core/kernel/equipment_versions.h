#pragma once

#include <string>
#include <vector>

namespace AqualinkAutomate::Kernel
{

	struct EquipmentVersionField
	{
		std::string label;
		std::string value;
	};

	struct EquipmentVersions
	{
		// Ordered list of label/value pairs.  Jandy controllers populate
		// Model, Type, and Revision; other equipment can use any labels.
		std::vector<EquipmentVersionField> Fields;

		// Convenience helpers for common lookups.
		void Set(const std::string& label, const std::string& value);
		std::string Get(const std::string& label) const;

		// Back-compat accessors used by existing code.
		std::string ModelNumber() const;
		std::string FirmwareRevision() const;
	};

}
// namespace AqualinkAutomate::Kernel

