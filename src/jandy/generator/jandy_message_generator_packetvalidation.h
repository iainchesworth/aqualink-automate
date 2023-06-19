#pragma once

#include <cstddef>
#include <span>

namespace AqualinkAutomate::Generators
{

	bool PacketValidation_ChecksumIsValid(const std::span<const std::byte>& message_span);

}
// namespace AqualinkAutomate::Generators
