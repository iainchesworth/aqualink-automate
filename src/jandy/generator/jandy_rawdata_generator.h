#pragma once

#include <cstdint>
#include <vector>

#include "jandy/types/jandy_types.h"

namespace AqualinkAutomate::Generators
{

	void GenerateRawDataFromMessage(const Types::JandyMessageTypePtr& msg, std::vector<uint8_t>& serial_data);

}
// namespace AqualinkAutomate::Generators
