#pragma once

#include <string>

#include "kernel/device_graph/device_graph_types.h"

namespace AqualinkAutomate::Kernel
{

	class DeviceLabelFilter
	{
	public:
		DeviceLabelFilter(const DevicesGraphType& graph, const std::string& device_label);

	public:
		bool operator()(const DevicesGraphType::edge_descriptor) const;
		bool operator()(const DevicesGraphType::vertex_descriptor vd) const;

	private:
		const DevicesGraphType& m_Graph;
		const std::string& m_DeviceLabel;
	};

}
// namespace AqualinkAutomate::Kernel
