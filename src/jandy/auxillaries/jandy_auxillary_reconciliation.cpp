#include "auxillaries/jandy_auxillary_reconciliation.h"

#include <magic_enum/magic_enum.hpp>

#include "auxillaries/jandy_auxillary_traits_types.h"
#include "kernel/auxillary_devices/auxillary_device.h"
#include "kernel/auxillary_traits/auxillary_traits_types.h"
#include "kernel/device_graph/device_graph.h"

namespace AqualinkAutomate::Auxillaries
{

	void EnsureAuxIdentity(const std::shared_ptr<Kernel::AuxillaryDevice>& device, JandyAuxillaryIds id)
	{
		if (nullptr == device)
		{
			return;
		}

		if (!device->AuxillaryTraits.Has(JandyAuxillaryId{}))
		{
			device->AuxillaryTraits.Set(JandyAuxillaryId{}, id);
		}

		if (!device->AuxillaryTraits.Has(Kernel::AuxillaryTraitsTypes::HardwareLabelTrait{}))
		{
			device->AuxillaryTraits.Set(Kernel::AuxillaryTraitsTypes::HardwareLabelTrait{}, std::string{ magic_enum::enum_name(id) });
		}
	}

	void RemoveOrphanAuxPlaceholders(Kernel::DevicesGraph& devices, const std::string& label, const std::shared_ptr<Kernel::AuxillaryDevice>& keep)
	{
		using Kernel::AuxillaryTraitsTypes::AuxillaryTypeTrait;
		using Kernel::AuxillaryTraitsTypes::AuxillaryTypes;

		// FindByLabel returns a snapshot vector, so removing from the graph mid-iteration is safe.
		for (const auto& orphan : devices.FindByLabel(label))
		{
			if (nullptr == orphan || orphan == keep)
			{
				continue;
			}
			// Only prune cache placeholders: an auxillary with no stable aux identity.
			if (orphan->AuxillaryTraits.Has(JandyAuxillaryId{}))
			{
				continue;
			}
			if (!orphan->AuxillaryTraits.Has(AuxillaryTypeTrait{}) ||
				AuxillaryTypes::Auxillary != *(orphan->AuxillaryTraits[AuxillaryTypeTrait{}]))
			{
				continue;
			}
			devices.Remove(orphan);
		}
	}

}
// namespace AqualinkAutomate::Auxillaries
