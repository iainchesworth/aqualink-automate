#include "jandy/errors/jandy_errors_scrapeable.h"

namespace AqualinkAutomate::ErrorCodes
{
	const Scrapeable_ErrorCategory& Scrapeable_ErrorCategory::Instance() {
		static Scrapeable_ErrorCategory category;
		return category;
	}

	const char* Scrapeable_ErrorCategory::name() const noexcept
	{
		return "AqualinkAutomate::Scrapeable Error Category";
	}

	std::string Scrapeable_ErrorCategory::message(int ev) const
	{
		switch (ev)
		{
		case Scrapeable_ErrorCodes::WaitingForPage:
			return "Scrapeable_ErrorCodes::WaitingForPage";

		case Scrapeable_ErrorCodes::WaitingForMessage:
			return "Scrapeable_ErrorCodes::WaitingForMessage";

		case Scrapeable_ErrorCodes::NoStepPossible:
			return "Scrapeable_ErrorCodes::NoStepPossible";

		case Scrapeable_ErrorCodes::NoGraphBeingScraped:
			return "Scrapeable_ErrorCodes::NoGraphBeingScraped";

		default:
			return "Scrapeable_ErrorCodes - Unknown Error Code";
		}
	}

}
// namespace AqualinkAutomate::ErrorCodes

boost::system::error_code make_error_code(const AqualinkAutomate::ErrorCodes::Scrapeable_ErrorCodes e)
{
	return boost::system::error_code(static_cast<int>(e), AqualinkAutomate::ErrorCodes::Scrapeable_ErrorCategory::Instance());
}

boost::system::error_condition make_error_condition(const AqualinkAutomate::ErrorCodes::Scrapeable_ErrorCodes e)
{
	return boost::system::error_condition(static_cast<int>(e), AqualinkAutomate::ErrorCodes::Scrapeable_ErrorCategory::Instance());
}
