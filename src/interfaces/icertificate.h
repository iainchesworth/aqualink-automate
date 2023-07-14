#pragma once

#include <string>

#include <boost/asio/buffer.hpp>

namespace AqualinkAutomate::Interfaces
{

	class ICertificate
	{
	public:
		ICertificate();
		ICertificate(const ICertificate& other);

	public:
		ICertificate& operator=(const ICertificate& other);

	public:
		virtual bool IsFile() const = 0;
		virtual bool IsValid() const = 0;

	public:
		std::string Path() const;
		boost::asio::const_buffer Data() const;

	protected:
		std::string m_Path;
		std::string m_Data;
	};

}
// namespace AqualinkAutomate::Interfaces
