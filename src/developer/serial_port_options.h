#pragma once 

#include <boost/asio/serial_port.hpp>

namespace AqualinkAutomate::Developer
{

	struct SerialPortOptions
	{
		boost::asio::serial_port::baud_rate baud_rate{ 9600 };
		boost::asio::serial_port::parity parity{ boost::asio::serial_port_base::parity::none };
		boost::asio::serial_port::character_size character_size{ 8 };
		boost::asio::serial_port::stop_bits stop_bits{ boost::asio::serial_port_base::stop_bits::one };
		boost::asio::serial_port::flow_control flow_control{ boost::asio::serial_port::flow_control::none };
	};

}
// namespace AqualinkAutomate::Developer
