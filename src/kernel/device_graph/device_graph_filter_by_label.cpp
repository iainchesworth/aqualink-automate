#include "kernel/auxillary_traits/auxillary_traits_types.h"
#include "kernel/device_graph/device_graph_filter_by_label.h"

namespace AqualinkAutomate::Kernel
{

	DeviceLabelFilter::DeviceLabelFilter(const DevicesGraphType& graph, const std::string& device_label) :
		m_Graph(graph),
		m_DeviceLabel(device_label)
	{
	}

	bool DeviceLabelFilter::operator()(const DevicesGraphType::edge_descriptor) const
	{
		return false;
	}

	bool DeviceLabelFilter::operator()(const DevicesGraphType::vertex_descriptor vd) const
	{
		using AuxillaryTraitsTypes::LabelTrait;

		if (auto device = m_Graph[vd]; nullptr == device)
		{
			// Invalid device pointer
		}
		else if (!(device->AuxillaryTraits.Has(LabelTrait{})))
		{
			// No label...does not match.
		}
		else if (m_DeviceLabel != LabelTrait::TraitValue{device->AuxillaryTraits[LabelTrait{}]})
		{
			// Label does not match.
		}
		else
		{
			return true;
		}

		return false;
	}

}
// namespace AqualinkAutomate::Kernel
