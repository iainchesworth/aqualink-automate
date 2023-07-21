#pragma once

#include <boost/uuid/uuid.hpp>

#include "kernel/device_graph/device_graph_types.h"

namespace AqualinkAutomate::Kernel
{

	class DeviceIdFilter
	{
	public:
		DeviceIdFilter(const DevicesGraphType& graph, const boost::uuids::uuid& device_id);

	public:
		bool operator()(const DevicesGraphType::edge_descriptor) const;
		bool operator()(const DevicesGraphType::vertex_descriptor vd) const;

	private:
		const DevicesGraphType& m_Graph;
		const boost::uuids::uuid& m_DeviceId;
	};

}
// namespace AqualinkAutomate::Kernel
