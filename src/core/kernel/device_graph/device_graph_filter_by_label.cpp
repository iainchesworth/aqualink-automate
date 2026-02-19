#include "kernel/auxillary_traits/auxillary_traits_types.h"
#include "kernel/device_graph/device_graph_filter_by_label.h"

namespace AqualinkAutomate::Kernel
{

	DeviceLabelFilter::DeviceLabelFilter(const DevicesGraphType& graph, std::string_view device_label) :
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

		auto device = m_Graph[vd];

		// Valid device pointer, has label, and label matches.
		return nullptr != device && device->AuxillaryTraits.Has(LabelTrait{}) && m_DeviceLabel == LabelTrait::TraitValue{device->AuxillaryTraits[LabelTrait{}]};
	}

}
// namespace AqualinkAutomate::Kernel
