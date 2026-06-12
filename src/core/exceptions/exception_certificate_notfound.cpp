#include "exceptions/exception_certificate_notfound.h"
#include "logging/logging.h"

namespace AqualinkAutomate::Exceptions
{

	AQ_DEFINE_EXCEPTION(Certificate_NotFound, "The configured TLS certificate or key file could not be found.");

}
// namespace AqualinkAutomate::Exceptions
