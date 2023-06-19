#pragma once

#include <cstdint>
#include <vector>

namespace AqualinkAutomate::Generators
{

	bool BufferValidation_ContainsMoreThanZeroBytes(const std::vector<uint8_t>& serial_data);
	bool BufferValidation_HasStartOfPacket(const std::vector<uint8_t>& serial_data);

}
// namespace AqualinkAutomate::Generators
