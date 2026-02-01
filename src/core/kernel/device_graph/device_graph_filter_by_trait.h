#pragma once

#include <optional>

#include "kernel/device_graph/device_graph_types.h"

namespace AqualinkAutomate::Kernel
{

	template<typename TRAIT_TYPE>
	class DeviceTraitFilter
	{
	public:
		DeviceTraitFilter(const DevicesGraphType& graph, TRAIT_TYPE trait_type) :
			m_Graph(graph),
			m_TraitType(trait_type),
			m_OptTraitValue(std::nullopt)
		{
		}

		DeviceTraitFilter(const DevicesGraphType& graph, TRAIT_TYPE trait_type, TRAIT_TYPE::TraitValue trait_value) :
			m_Graph(graph),
			m_TraitType(trait_type),
			m_OptTraitValue(trait_value)
		{
		}

	public:
		bool operator()(const DevicesGraphType::edge_descriptor) const { return false; }
		bool operator()(const DevicesGraphType::vertex_descriptor vd) const
		{
			if (auto device = m_Graph[vd]; nullptr == device)
			{
				// Invalid device pointer
			}
			else if (!device->AuxillaryTraits.Has(m_TraitType))
			{
				// Trait does not exist
			}
			else if (m_OptTraitValue.has_value() && (m_OptTraitValue.value() != *(device->AuxillaryTraits[m_TraitType])))
			{
				// Trait value didn't match
			}
			else
			{
				// Didn't need to validate trait value OR trait value matched.
				return true;
			}

			return false;
		}

	private:
		const DevicesGraphType& m_Graph;
		TRAIT_TYPE m_TraitType;
		std::optional<typename TRAIT_TYPE::TraitValue> m_OptTraitValue;
	};

}
// namespace AqualinkAutomate::Kernel
