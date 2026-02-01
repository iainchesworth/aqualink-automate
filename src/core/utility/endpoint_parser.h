#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>

namespace AqualinkAutomate::Utility
{

    struct EndpointView
    {
        std::string_view host;                  // hostname or IP (IPv6 without brackets)
        std::string_view port;                  // port string (empty if none, or default_port if provided)
        std::optional<std::uint16_t> port_num;  // numeric port if valid (1..65535)
        std::string_view scope_id;              // IPv6 zone/scope id (percent-decoded if %25 prefix present)
        bool had_brackets = false;              // true if original input used [ipv6] format
    };

    EndpointView ParseEndpointView(std::string_view input, std::string_view default_port = {});
    std::tuple<std::string, std::string> ParseEndpoint(std::string_view input, std::string_view default_port = {});

}
// namespace AqualinkAutomate::Utility
