#include "exceptions/exception_certificate_invalidformat.h"
#include "logging/logging.h"

namespace AqualinkAutomate::Exceptions
{

	AQ_DEFINE_EXCEPTION(Certificate_InvalidFormat, "The TLS certificate or key file is not in a valid PEM format.");

}
// namespace AqualinkAutomate::Exceptions
