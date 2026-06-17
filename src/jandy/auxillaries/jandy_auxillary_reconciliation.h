#pragma once

#include <memory>
#include <string>

#include "auxillaries/jandy_auxillary_id.h"

namespace AqualinkAutomate::Kernel
{
	class AuxillaryDevice;
	class DevicesGraph;
}
// namespace AqualinkAutomate::Kernel

namespace AqualinkAutomate::Auxillaries
{

	// Ensure a (possibly cache-restored) device carries the protocol identity for an aux:
	// sets JandyAuxillaryId and the immutable HardwareLabelTrait ONLY IF ABSENT. Never
	// re-sets an existing immutable trait (which would throw Traits_NotMutable), so it is
	// safe to call on every reconciliation. Cache-restored devices arrive with the stable id
	// and label but WITHOUT JandyAuxillaryId (the cache is protocol-agnostic) - this grants
	// it so actuation/power-center mapping work.
	void EnsureAuxIdentity(const std::shared_ptr<Kernel::AuxillaryDevice>& device, JandyAuxillaryIds id);

	// Remove cache-restored placeholders that share `label`, are auxillaries, and lack a
	// stable aux identity (pre-upgrade caches persisted only the label, so they cannot be
	// matched by stable id and would otherwise survive as duplicates). `keep` - the live
	// device that now owns the identity - is never removed. One-time cleanup that fires on the
	// first restart after upgrading from a label-only cache; a no-op thereafter.
	void RemoveOrphanAuxPlaceholders(Kernel::DevicesGraph& devices, const std::string& label, const std::shared_ptr<Kernel::AuxillaryDevice>& keep);

}
// namespace AqualinkAutomate::Auxillaries
