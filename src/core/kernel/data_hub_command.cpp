#include "kernel/data_hub_command.h"

namespace AqualinkAutomate::Kernel
{

	DataHub_Command::DataHub_Command(DataHub_CommandTypes command_type) :
		m_CommandType(command_type)
	{
	}

	DataHub_CommandTypes DataHub_Command::Type() const
	{
		return m_CommandType;
	}

}
// namespace AqualinkAutomate::Kernel
