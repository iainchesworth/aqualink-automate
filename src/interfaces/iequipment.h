#pragma once

#include "types/asynchronous_executor.h"

namespace AqualinkAutomate::Interfaces
{
	class IEquipment
	{
	public:
		IEquipment(Types::ExecutorType executor);
		virtual ~IEquipment();

	protected:
		Types::ExecutorType m_Executor;
	};

}
// namespace AqualinkAutomate::Interfaces
