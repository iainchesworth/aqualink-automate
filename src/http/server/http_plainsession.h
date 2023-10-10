#pragma once

#include <memory>

#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core/tcp_stream.hpp>

#include "http/server/http_session.h"

namespace AqualinkAutomate::HTTP
{

	class HTTP_PlainSession : public HTTP_Session<HTTP_PlainSession>, public std::enable_shared_from_this<HTTP_PlainSession>
	{
	public:
        HTTP_PlainSession(boost::beast::tcp_stream&& stream, boost::beast::flat_buffer&& buffer);

	public:
		void Run();

	public:
		boost::beast::tcp_stream& Stream();
		boost::beast::tcp_stream ReleaseStream();

	public:
		void DoEOF();

	private:
		boost::beast::tcp_stream m_Stream;
	};

}
// namespace AqualinkAutomate::HTTP
