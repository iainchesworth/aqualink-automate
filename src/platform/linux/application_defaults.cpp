#include "application/application_defaults.h"

namespace AqualinkAutomate::Application
{

	const std::string SERIAL_PORT{ "/dev/ttyS0" };
	const std::string DOC_ROOT{ "templates/" };
	const uint16_t DEFAULT_HTTP_PORT{ 80 };
	const uint16_t DEFAULT_HTTPS_PORT{ 443 };
	const std::string DEFAULT_CERTIFICATE{ "ssl/cert.pem" };
	const std::string DEFAULT_PRIVATE_KEY{ "ssl/key.pem" };

}
// namespace AqualinkAutomate::Application
