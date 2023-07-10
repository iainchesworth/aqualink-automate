#pragma once

#include <filesystem>
#include <format>
#include <string>
#include <unordered_map>

#include "concepts/is_c_array.h"
#include "http/webroute_types.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Interfaces
{

	template<const auto& ROUTE_URL, const auto& TEMPLATE_FILENAME>
	requires (Concepts::CArrayRef<decltype(ROUTE_URL)> && Concepts::CArrayRef<decltype(TEMPLATE_FILENAME)>)
	class IWebPageRoute
	{
	public:
		explicit IWebPageRoute(HTTP::Server& http_server)
		{
			http_server.set_http_handler<HTTP::Methods::GET>(ROUTE_URL, 
				[this](HTTP::Request& req, HTTP::Response& resp) -> void
				{
					WebRequestHandler(req, resp);
				}
			);
		}

	protected:
		virtual void WebRequestHandler(HTTP::Request& req, HTTP::Response& resp) = 0;

	protected:
		static std::string LoadTemplateFromFile(const char * path)
		{
			std::string ret;

			if (!std::filesystem::exists(path))
			{
				LogWarning(Channel::Web, std::format("Requested template ({}) does not exist at the location specified", path));
			}
			else if (auto const fd = std::fopen(path, "rb"); nullptr == fd)
			{
				LogWarning(Channel::Web, std::format("Failed to open specified template: {}", path));
			}
			else
			{
				auto const bytes = std::filesystem::file_size(path);
				ret.resize(bytes);
				std::fread(ret.data(), 1, bytes, fd);
				std::fclose(fd);
			}

			return ret;
		};
	};

}
// namespace AqualinkAutomate::Interfaces
