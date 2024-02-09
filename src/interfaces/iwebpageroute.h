#pragma once

#include <filesystem>
#include <format>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

#include <mstch/mstch.hpp>

#include "concepts/is_c_array.h"
#include "http/server/server_fields.h"
#include "http/server/responses/response_405.h"
#include "http/server/responses/response_500.h"
#include "http/server/responses/response_503.h"
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
		virtual HTTP::Message OnRequest(const HTTP::Request& req) final
		{
			auto generate_page = [this](const HTTP::Request& req) -> HTTP::Message
				{
					try
					{
						HTTP::Response resp{ HTTP::Status::ok, req.version() };

						resp.set(boost::beast::http::field::server, HTTP::ServerFields::Server());
						resp.set(boost::beast::http::field::content_type, HTTP::ContentTypes::TEXT_HTML);
						resp.keep_alive(req.keep_alive());
						resp.body() = GenerateBody(req);
						resp.prepare_payload();

						return resp;
					}
					catch (const std::bad_optional_access& /* unused */)
					{						
						if (!m_TemplateContent.has_value())
						{
							// One of the page generators likely failed due to a missing template
							// during the rendering of the mstch.

							static const std::string_view REASON_MISSING_TEMPLATE{ "Could not generate page content; template is missing/not loaded" };
							return HTTP::Responses::Response_503(req, REASON_MISSING_TEMPLATE);
						}

						return HTTP::Responses::Response_500(req);
					}
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
		static std::optional<std::string> LoadTemplateFromFile(const char * path)
		{
			std::error_code ec;
			std::optional<std::string> ret{ std::nullopt };

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
					std::string loaded_template;

					loaded_template.resize(bytes);

					if (auto bytes_read = std::fread(loaded_template.data(), 1, bytes, fd); bytes != bytes_read)
					{
						LogWarning(Channel::Web, std::format("Failed to read the specified template content: expected {} bytes; actual {} bytes", bytes, bytes_read));
					}
					else
					{
						ret = std::move(loaded_template);
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
		std::optional<std::string> m_TemplateContent{ std::nullopt };
		mstch::map m_TemplateContext{};
	};

}
// namespace AqualinkAutomate::Interfaces
