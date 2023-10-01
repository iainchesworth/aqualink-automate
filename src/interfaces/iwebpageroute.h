#pragma once

#include <filesystem>
#include <format>
#include <memory>
#include <string>
#include <unordered_map>

#include <mstch/mstch.hpp>

#include "concepts/is_c_array.h"
#include "http/server/responses/response_405.h"
#include "interfaces/iwebroute.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Interfaces
{

	template<const auto& ROUTE_URL, const auto& TEMPLATE_FILENAME>
	requires (Concepts::CArray<decltype(ROUTE_URL)> && Concepts::CArrayRef<decltype(TEMPLATE_FILENAME)>)
	class IWebPageRoute : public IWebRoute<ROUTE_URL>
	{
	public:
		explicit IWebPageRoute() : 
			IWebRoute<ROUTE_URL>(),
			m_TemplateContent(LoadTemplateFromFile(TEMPLATE_FILENAME))
		{
		}

		virtual ~IWebPageRoute() = default;

	public:
		virtual HTTP::Message OnRequest(HTTP::Request req) final
		{
			auto generate_page = [this](HTTP::Request req) -> HTTP::Message
				{
					HTTP::Response resp{ HTTP::Status::ok, req.version() };

					resp.set(boost::beast::http::field::server, "1.2.3.4");
					resp.set(boost::beast::http::field::content_type, "text/html");
					resp.keep_alive(req.keep_alive());
					resp.body() = GenerateBody(req);
					resp.prepare_payload();

					return resp;
				};

			switch (req.method())
			{
			case HTTP::Verbs::get:
				return generate_page(req);

			default:
				return HTTP::Responses::Response_405(req);
			}
		}

	protected:
		virtual std::string GenerateBody(HTTP::Request req) = 0;

	protected:
		static std::string LoadTemplateFromFile(const char * path)
		{
			std::error_code ec;
			std::string ret;

			if (!std::filesystem::exists(path))
			{
				LogWarning(Channel::Web, std::format("Requested template ({}) does not exist at the location specified", path));
			}
			else if (auto const fd = std::fopen(path, "rb"); nullptr == fd)
			{
				LogWarning(Channel::Web, std::format("Failed to open specified template: {}", path));
			}
			else if (auto const bytes = std::filesystem::file_size(path, ec); ec)
			{
				LogWarning(Channel::Web, std::format("Failed to get the file size for specified template: {}", path));
				std::fclose(fd);
			}
			else
			{
				try 
				{
					ret.resize(bytes);

					if (auto bytes_read = std::fread(ret.data(), 1, bytes, fd); bytes != bytes_read)
					{
						LogWarning(Channel::Web, std::format("Failed to read the specified template content: expected {} bytes; actual {} bytes", bytes, bytes_read));
						ret.clear();
					}

					std::fclose(fd);
				}
				catch (const std::bad_alloc& ex_ba)
				{
					LogWarning(Channel::Web, std::format("Failed to allocate memory for specified template: {}", path));
					std::fclose(fd);
				}				
			}

			return ret;
		};

	protected:
		std::string m_TemplateContent{};
		mstch::map m_TemplateContext{};
	};

}
// namespace AqualinkAutomate::Interfaces
