#pragma once

#include "interfaces/iwebroute.h"

namespace AqualinkAutomate::HTTP
{
	inline constexpr char DIAGNOSTICS_LOGGING_ROUTE_URL[] = "/api/diagnostics/logging";

	class WebRoute_Diagnostics_Logging : public Interfaces::IWebRoute<DIAGNOSTICS_LOGGING_ROUTE_URL>
	{
	public:
		WebRoute_Diagnostics_Logging() = default;
		~WebRoute_Diagnostics_Logging() override = default;

	public:
		HTTP::Response OnRequest(const HTTP::Request& req) final;

	private:
		HTTP::Response HandleGet(const HTTP::Request& req);
		HTTP::Response HandlePost(const HTTP::Request& req);
	};

}
// namespace AqualinkAutomate::HTTP
