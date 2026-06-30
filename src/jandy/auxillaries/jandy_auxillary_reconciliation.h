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

	// Remove cache-restored placeholders for the auxillary identified by `aux_id` that lack a
	// stable aux identity, collapsing them onto `keep` (the live device that owns the identity,
	// never removed). A placeholder "belongs" to this aux when its label OR hardware label parses
	// to `aux_id`, or when it shares `keep`'s current label. Matching on the aux identity rather
	// than only the (custom) label is what lets this fire at the FIRST live touch of the aux -
	// before the custom label is even known - so a legacy random-id placeholder never reaches the
	// MQTT/HA publishers as a duplicate. Any custom label / body-of-water the placeholder carries
	// but `keep` does not yet have is transferred to `keep` before removal, so pruning early never
	// loses cache-enumerated information.
	void RemoveOrphanAuxPlaceholders(Kernel::DevicesGraph& devices, JandyAuxillaryIds aux_id, const std::shared_ptr<Kernel::AuxillaryDevice>& keep);

}
// namespace AqualinkAutomate::Auxillaries
