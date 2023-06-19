#include "interfaces/icertificate.h"

namespace AqualinkAutomate::Interfaces
{

	ICertificate::ICertificate() :
		m_Path(),
		m_Data()
	{
	}

	ICertificate::ICertificate(const ICertificate& other) :
		m_Path(other.m_Path),
		m_Data(other.m_Data)
	{
	}

	ICertificate& ICertificate::operator=(const ICertificate& other)
	{
		if (this != &other)
		{
			m_Path = other.m_Path;
			m_Data = other.m_Data;
		}

		return *this;
	}

	std::string ICertificate::Path() const
	{
		return m_Path;
	}

	boost::asio::const_buffer ICertificate::Data() const
	{
		return boost::asio::const_buffer(m_Data.data(), m_Data.size());
	}

}
// namespace AqualinkAutomate::Interfaces
