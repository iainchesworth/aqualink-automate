#pragma once

#include <boost/uuid/uuid.hpp>

namespace AqualinkAutomate::Kernel
{

	enum class DataHub_CommandTypes
	{
		BLAH
	};

	class DataHub_Command
	{
	public:
		DataHub_Command(DataHub_CommandTypes command_type);

	public:
		DataHub_CommandTypes Type() const;

	public:
		virtual boost::uuids::uuid Id() const = 0;

	private:
		DataHub_CommandTypes m_CommandType;
	};

}
// namespace AqualinkAutomate::Kernel
