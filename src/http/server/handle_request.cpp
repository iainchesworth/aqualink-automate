#include <boost/url/parse_path.hpp>

#include "http/server/handle_request.h"
#include "http/server/router/matches.h"

namespace AqualinkAutomate::HTTP
{

	HTTP::Message HandleRequest(std::shared_ptr<HTTP::Router::Router> router, HTTP::Request const& req)
	{	
		if (auto rpath = boost::urls::parse_path(req.target()); !rpath)
		{
			///FIXME
		}
		else if (HTTP::Router::matches m; auto h = router.find(*rpath, m))
		{
			return (*h)(c, m);
		}
		else
		{
			///FIXME
		}

		throw;
	}

}
// namespace AqualinkAutomate::HTTP
