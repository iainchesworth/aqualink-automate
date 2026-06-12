#pragma once

#include <nlohmann/json.hpp>

namespace AqualinkAutomate::Interfaces
{

	class IDescribable
	{
	public:
		virtual ~IDescribable() = default;
		virtual nlohmann::json DescribeDiagnostics() const = 0;
	};

}
// namespace AqualinkAutomate::Interfaces
