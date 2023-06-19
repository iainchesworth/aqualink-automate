#include "certificates/certificate_staticdata.h"

namespace AqualinkAutomate::Certificates
{

	const std::string Certificate_StaticData::STATIC_CERTIFICATE_DATA{ "STATIC CERTIFICATE DATA" };

	Certificate_StaticData::Certificate_StaticData(const std::string& certificate_data) :
		ICertificate()
	{
		if (certificate_data.empty())
		{
			///FIXME
		}
		else
		{
			m_Data = certificate_data;
			m_Path = STATIC_CERTIFICATE_DATA;
			m_Valid = true;
		}
	}

	bool Certificate_StaticData::IsFile() const
	{
		return false;
	}

	bool Certificate_StaticData::IsValid() const 
	{
		return m_Valid;
	}
	
}
// namespace AqualinkAutomate::Certificates
