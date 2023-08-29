#pragma once

#include <boost/asio/io_context.hpp>

namespace AqualinkAutomate::Interfaces
{
	class IEquipment
	{
	public:
		IEquipment(boost::asio::io_context& io_context);
		virtual ~IEquipment();
	
	protected:
		boost::asio::io_context& m_IOContext;
	};

}
// namespace AqualinkAutomate::Interfaces
