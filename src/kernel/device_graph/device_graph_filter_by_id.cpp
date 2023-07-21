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
		if (auto device = m_Graph[vd]; nullptr == device)
		{
			// Invalid device pointer
		}
		else if (device->Id() != m_DeviceId)
		{
			// Id does not match.
		}
		else
		{
			return true;
		}

		return false;
	}

}
// namespace AqualinkAutomate::Kernel
