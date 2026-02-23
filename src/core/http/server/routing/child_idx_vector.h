#pragma once

#include <cstddef>
#include <vector>

namespace AqualinkAutomate::HTTP::Routing
{

    // a small vector for child nodes...we shouldn't expect many children per node, and we don't want to allocate for that. But we also
    // cannot cap the max number of child nodes because especially the root nodes can potentially an exponentially higher number of child nodes.

    using ChildIdxVector = std::vector<std::size_t>;

}
// namespace AqualinkAutomate::HTTP::Routing
