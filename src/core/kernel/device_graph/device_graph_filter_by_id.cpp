#include "kernel/device_graph/device_graph_filter_by_id.h"

namespace AqualinkAutomate::Kernel
{

	DeviceIdFilter::DeviceIdFilter(const DevicesGraphType& graph, const boost::uuids::uuid& device_id) :
		m_Graph(graph),
		m_DeviceId(device_id)
	{
	}

	bool DeviceIdFilter::operator()(const DevicesGraphType::edge_descriptor) const
	{
		return false;
	}

	bool DeviceIdFilter::operator()(const DevicesGraphType::vertex_descriptor vd) const
	{
		auto device = m_Graph[vd];

		// Valid device pointer and Id matches.
		return nullptr != device && device->Id() == m_DeviceId;
	}

}
// namespace AqualinkAutomate::Kernel
