#pragma once

#include "interfaces/iwebroute.h"

namespace AqualinkAutomate::HTTP
{
	inline constexpr char DIAGNOSTICS_OPTIONS_ROUTE_URL[] = "/api/diagnostics/options";

	class WebRoute_Diagnostics_Options : public Interfaces::IWebRoute<DIAGNOSTICS_OPTIONS_ROUTE_URL>
	{
	public:
		WebRoute_Diagnostics_Options() = default;
		~WebRoute_Diagnostics_Options() override = default;

	public:
		HTTP::Message OnRequest(const HTTP::Request& req) final;

	private:
		HTTP::Message HandleGet(const HTTP::Request& req);
	};

}
// namespace AqualinkAutomate::HTTP
