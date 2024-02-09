#pragma once

#include "types/asynchronous_executor.h"

namespace AqualinkAutomate::Interfaces
{
	class IEquipment
	{
	public:
		IEquipment(Types::ExecutorType executor);
		virtual ~IEquipment() = default;

	protected:
		Types::ExecutorType m_Executor;
	};

}
// namespace AqualinkAutomate::Interfaces
