#include <boost/test/unit_test.hpp>

#include "support/unit_test_onetouchdevice_httpserver.h"

namespace AqualinkAutomate::Test
{

	Test_OneTouchDevicePlusHttpServer::Test_OneTouchDevicePlusHttpServer() :
		Test::OneTouchDevice(),
		m_API_Equipment(*this),
		m_WS_Equipment(*this)
	{
	}

	Test_OneTouchDevicePlusHttpServer::~Test_OneTouchDevicePlusHttpServer()
	{
	}

	void Test_OneTouchDevicePlusHttpServer::ReadFromHttpApi_NonBlocking(const std::string& api_route, boost::beast::flat_buffer& buffer)
	{
	}

	void Test_OneTouchDevicePlusHttpServer::ReadFromWebSocket_NonBlocking(boost::beast::flat_buffer& buffer)
	{
	}

}
// namespace AqualinkAutomate::Test
