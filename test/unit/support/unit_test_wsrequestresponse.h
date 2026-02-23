#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <string_view>
#include <tuple>

namespace AqualinkAutomate::Test
{

	std::tuple<std::array<uint8_t, 1024>, std::size_t> PerformHttpWsUpgradeResponse(const std::string_view& url_to_retrieve, std::function<void()> on_ws_connected);

}
// namespace AqualinkAutomate::Test
