#pragma once

#include <cstdint>
#include <memory>
#include <unordered_map>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>

#include "kernel/auxillary_base.h"

namespace AqualinkAutomate::Kernel
{

	using DeviceVertexProperty = std::shared_ptr<AuxillaryBase>;
	using DevicesGraphType = boost::adjacency_list<boost::listS, boost::vecS, boost::directedS, DeviceVertexProperty>;
	using DeviceVertexType = boost::graph_traits<DevicesGraphType>::vertex_descriptor;
	using DeviceVertexIter = boost::graph_traits<DevicesGraphType>::vertex_iterator;
	using DeviceMap = std::unordered_map<uint32_t, std::shared_ptr<AuxillaryBase>>;

}
// namespace AqualinkAutomate::Kernel
