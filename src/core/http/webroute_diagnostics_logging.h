#pragma once

#include "interfaces/iwebroute.h"

namespace AqualinkAutomate::HTTP
{
	inline constexpr char DIAGNOSTICS_LOGGING_ROUTE_URL[] = "/api/diagnostics/logging";

	class WebRoute_Diagnostics_Logging : public Interfaces::IWebRoute<DIAGNOSTICS_LOGGING_ROUTE_URL>
	{
	public:
		WebRoute_Diagnostics_Logging() = default;
		virtual ~WebRoute_Diagnostics_Logging() = default;

	public:
		virtual HTTP::Message OnRequest(const HTTP::Request& req) final;

	private:
		HTTP::Message HandleGet(const HTTP::Request& req);
		HTTP::Message HandlePost(const HTTP::Request& req);
	};

}
// namespace AqualinkAutomate::HTTP
