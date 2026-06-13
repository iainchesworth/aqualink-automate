#pragma once

#include <cstdint>
#include <vector>

#include "auxillaries/jandy_auxillary_id.h"
#include "kernel/equipment_validation.h"

namespace AqualinkAutomate::Utility
{

	// Cross-check the discovered equipment against the model's expected layout.
	//
	//   expected_aux              - aux relay count for the model (PoolConfigurationDecoder).
	//   expected_power_centers    - power-centre count for the model.
	//   discovered_aux_ids        - Jandy ids of every numbered aux found (ExtraAux is ignored
	//                               for counting; it is a dedicated shared relay).
	//   reconfigured_aux_relay_count - equipment occupying an aux relay that is NOT a numbered
	//                               aux because a DIP switch repurposed the relay (cleaner /
	//                               spillover / sprinkler). Counted toward the relay total so a
	//                               DIP-repurposed panel still validates against the model.
	//
	// Returns a summary with any anomalies (empty == clean). Pure function so it is exercised
	// directly by unit tests; the OneTouch device gathers the inputs from the DataHub.
	Kernel::EquipmentValidation ValidateDiscoveredEquipment(
		uint8_t expected_aux,
		uint8_t expected_power_centers,
		const std::vector<Auxillaries::JandyAuxillaryIds>& discovered_aux_ids,
		uint8_t reconfigured_aux_relay_count);

}
// namespace AqualinkAutomate::Utility
