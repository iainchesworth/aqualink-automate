#include <boost/test/unit_test.hpp>

#include "utility/endpoint_parser.h"

#include "support/unit_test_ostream_support.h"

using namespace AqualinkAutomate::Utility;

BOOST_AUTO_TEST_SUITE(TestSuite_EndpointParser);

BOOST_AUTO_TEST_CASE(Test_EndpointParser_EmptyInput)
{
    {
        auto v = ParseEndpointView("");
        BOOST_TEST(v.host.empty());
        BOOST_TEST(v.port.empty());
        BOOST_TEST(!v.port_num.has_value());
        BOOST_TEST(v.scope_id.empty());
        BOOST_TEST(!v.had_brackets);
    }
    {
        auto v = ParseEndpointView("   \t\n  ");
        BOOST_TEST(v.host.empty());
        BOOST_TEST(v.port.empty());
        BOOST_TEST(!v.port_num.has_value());
    }
    {
        // Empty with default port
        auto v = ParseEndpointView("", "8080");
        BOOST_TEST(v.host.empty());
        BOOST_TEST(v.port == "8080");
        BOOST_REQUIRE(v.port_num.has_value());
        BOOST_TEST(*v.port_num == 8080u);
    }
    {
        // Whitespace with default port
        auto v = ParseEndpointView("  \t  ", "443");
        BOOST_TEST(v.host.empty());
        BOOST_TEST(v.port == "443");
        BOOST_REQUIRE(v.port_num.has_value());
        BOOST_TEST(*v.port_num == 443u);
    }
}

BOOST_AUTO_TEST_CASE(Test_EndpointParser_HostnameBasic)
{
    {
        auto v = ParseEndpointView("example.com:8080");
        BOOST_TEST(v.host == "example.com");
        BOOST_TEST(v.port == "8080");
        BOOST_REQUIRE(v.port_num.has_value());
        BOOST_TEST(*v.port_num == 8080u);
        BOOST_TEST(!v.had_brackets);
    }
    {
        auto v = ParseEndpointView("example.org");
        BOOST_TEST(v.host == "example.org");
        BOOST_TEST(v.port.empty());
        BOOST_TEST(!v.port_num.has_value());
    }
    {
        // Hostname with hyphen and numbers
        auto v = ParseEndpointView("my-server-123.example.com:9000");
        BOOST_TEST(v.host == "my-server-123.example.com");
        BOOST_TEST(v.port == "9000");
        BOOST_REQUIRE(v.port_num.has_value());
        BOOST_TEST(*v.port_num == 9000u);
    }
    {
        // Single label hostname
        auto v = ParseEndpointView("localhost:3000");
        BOOST_TEST(v.host == "localhost");
        BOOST_TEST(v.port == "3000");
        BOOST_REQUIRE(v.port_num.has_value());
        BOOST_TEST(*v.port_num == 3000u);
    }
}

BOOST_AUTO_TEST_CASE(Test_EndpointParser_IPv4)
{
    {
        auto v = ParseEndpointView("192.168.1.10:2101");
        BOOST_TEST(v.host == "192.168.1.10");
        BOOST_TEST(v.port == "2101");
        BOOST_REQUIRE(v.port_num.has_value());
        BOOST_TEST(*v.port_num == 2101u);
        BOOST_TEST(!v.had_brackets);
    }
    {
        auto v = ParseEndpointView("192.168.1.10");
        BOOST_TEST(v.host == "192.168.1.10");
        BOOST_TEST(v.port.empty());
        BOOST_TEST(!v.port_num.has_value());
    }
    {
        // Edge case: 0.0.0.0
        auto v = ParseEndpointView("0.0.0.0:80");
        BOOST_TEST(v.host == "0.0.0.0");
        BOOST_TEST(v.port == "80");
        BOOST_REQUIRE(v.port_num.has_value());
        BOOST_TEST(*v.port_num == 80u);
    }
    {
        // Edge case: 255.255.255.255
        auto v = ParseEndpointView("255.255.255.255:443");
        BOOST_TEST(v.host == "255.255.255.255");
        BOOST_TEST(v.port == "443");
        BOOST_REQUIRE(v.port_num.has_value());
        BOOST_TEST(*v.port_num == 443u);
    }
}

BOOST_AUTO_TEST_CASE(Test_EndpointParser_IPv6Bracketed)
{
    {
        auto v = ParseEndpointView("[::1]:443");
        BOOST_TEST(v.had_brackets);
        BOOST_TEST(v.host == "::1");
        BOOST_TEST(v.port == "443");
        BOOST_REQUIRE(v.port_num.has_value());
        BOOST_TEST(*v.port_num == 443u);
        BOOST_TEST(v.scope_id.empty());
    }
    {
        auto v = ParseEndpointView("[2001:db8::1]");
        BOOST_TEST(v.had_brackets);
        BOOST_TEST(v.host == "2001:db8::1");
        BOOST_TEST(v.port.empty());
        BOOST_TEST(!v.port_num.has_value());
    }
    {
        // Full IPv6 address
        auto v = ParseEndpointView("[2001:0db8:85a3:0000:0000:8a2e:0370:7334]:8080");
        BOOST_TEST(v.had_brackets);
        BOOST_TEST(v.host == "2001:0db8:85a3:0000:0000:8a2e:0370:7334");
        BOOST_TEST(v.port == "8080");
        BOOST_REQUIRE(v.port_num.has_value());
        BOOST_TEST(*v.port_num == 8080u);
    }
    {
        // IPv6 loopback
        auto v = ParseEndpointView("[::]:9999");
        BOOST_TEST(v.had_brackets);
        BOOST_TEST(v.host == "::");
        BOOST_TEST(v.port == "9999");
        BOOST_REQUIRE(v.port_num.has_value());
        BOOST_TEST(*v.port_num == 9999u);
    }
}

BOOST_AUTO_TEST_CASE(Test_EndpointParser_IPv6ZoneId)
{
    {
        auto v = ParseEndpointView("[fe80::1%25eth0]:5353");
        BOOST_TEST(v.had_brackets);
        BOOST_TEST(v.host == "fe80::1");
        BOOST_TEST(v.scope_id == "eth0");
        BOOST_TEST(v.port == "5353");
        BOOST_REQUIRE(v.port_num.has_value());
        BOOST_TEST(*v.port_num == 5353u);
    }
    {
        // Zone ID without port
        auto v = ParseEndpointView("[fe80::1%25en0]");
        BOOST_TEST(v.had_brackets);
        BOOST_TEST(v.host == "fe80::1");
        BOOST_TEST(v.scope_id == "en0");
        BOOST_TEST(v.port.empty());
        BOOST_TEST(!v.port_num.has_value());
    }
    {
        // Different zone ID formats
        auto v = ParseEndpointView("[fe80::abcd%25wlan0]:80");
        BOOST_TEST(v.had_brackets);
        BOOST_TEST(v.host == "fe80::abcd");
        BOOST_TEST(v.scope_id == "wlan0");
        BOOST_TEST(v.port == "80");
        BOOST_REQUIRE(v.port_num.has_value());
        BOOST_TEST(*v.port_num == 80u);
    }
    {
        // Zone ID with numbers
        auto v = ParseEndpointView("[fe80::1%252]:443");
        BOOST_TEST(v.had_brackets);
        BOOST_TEST(v.host == "fe80::1");
        BOOST_TEST(v.scope_id == "2");
        BOOST_TEST(v.port == "443");
        BOOST_REQUIRE(v.port_num.has_value());
        BOOST_TEST(*v.port_num == 443u);
    }
}

BOOST_AUTO_TEST_CASE(Test_EndpointParser_UnbracketedIPv6_NoSplit)
{
    {
        auto v = ParseEndpointView("2001:db8::1");
        BOOST_TEST(!v.had_brackets);
        BOOST_TEST(v.host == "2001:db8::1");
        BOOST_TEST(v.port.empty());
        BOOST_TEST(!v.port_num.has_value());
    }
    {
        // Full unbracketed IPv6
        auto v = ParseEndpointView("2001:0db8:85a3:0000:0000:8a2e:0370:7334");
        BOOST_TEST(!v.had_brackets);
        BOOST_TEST(v.host == "2001:0db8:85a3:0000:0000:8a2e:0370:7334");
        BOOST_TEST(v.port.empty());
        BOOST_TEST(!v.port_num.has_value());
    }
    {
        // IPv6 with :: compression
        auto v = ParseEndpointView("fe80::1");
        BOOST_TEST(!v.had_brackets);
        BOOST_TEST(v.host == "fe80::1");
        BOOST_TEST(v.port.empty());
        BOOST_TEST(!v.port_num.has_value());
    }
    {
        // Unbracketed IPv6 - no split even if last segment looks like port
        auto v = ParseEndpointView("2001:db8::1:abcd");
        BOOST_TEST(!v.had_brackets);
        BOOST_TEST(v.host == "2001:db8::1:abcd");
        BOOST_TEST(v.port.empty());
        BOOST_TEST(!v.port_num.has_value());
    }
    {
        // Unbracketed IPv6 ending with digits - still no split
        // Users MUST use brackets for IPv6 with port
        auto v = ParseEndpointView("2001:db8::1:8080");
        BOOST_TEST(!v.had_brackets);
        BOOST_TEST(v.host == "2001:db8::1:8080");
        BOOST_TEST(v.port.empty());
        BOOST_TEST(!v.port_num.has_value());
    }
    {
        // Short IPv6 with digit - no split
        auto v = ParseEndpointView("fe80::80");
        BOOST_TEST(!v.had_brackets);
        BOOST_TEST(v.host == "fe80::80");
        BOOST_TEST(v.port.empty());
        BOOST_TEST(!v.port_num.has_value());
    }
    {
        // Use brackets to specify port with IPv6
        auto v = ParseEndpointView("[fe80::1]:8080");
        BOOST_TEST(v.had_brackets);
        BOOST_TEST(v.host == "fe80::1");
        BOOST_TEST(v.port == "8080");
        BOOST_REQUIRE(v.port_num.has_value());
        BOOST_TEST(*v.port_num == 8080u);
    }
}

BOOST_AUTO_TEST_CASE(Test_EndpointParser_MalformedBrackets)
{
    {
        auto v = ParseEndpointView("[::1:443"); // missing ']'
        BOOST_TEST(v.had_brackets);               // started with '['
        BOOST_TEST(v.host == "::1:443");          // treated as whole host sans '['
        BOOST_TEST(v.port.empty());
        BOOST_TEST(!v.port_num.has_value());
    }
    {
        // Missing closing bracket with zone ID
        auto v = ParseEndpointView("[fe80::1%25eth0");
        BOOST_TEST(v.had_brackets);
        BOOST_TEST(v.host == "fe80::1%25eth0");
        BOOST_TEST(v.port.empty());
        BOOST_TEST(!v.port_num.has_value());
    }
    {
        // Opening bracket only
        auto v = ParseEndpointView("[");
        BOOST_TEST(v.had_brackets);
        BOOST_TEST(v.host.empty());
        BOOST_TEST(v.port.empty());
        BOOST_TEST(!v.port_num.has_value());
    }
    {
        // Empty brackets
        auto v = ParseEndpointView("[]");
        BOOST_TEST(v.had_brackets);
        BOOST_TEST(v.host.empty());
        BOOST_TEST(v.port.empty());
        BOOST_TEST(!v.port_num.has_value());
    }
    {
        // Empty brackets with port
        auto v = ParseEndpointView("[]:8080");
        BOOST_TEST(v.had_brackets);
        BOOST_TEST(v.host.empty());
        BOOST_TEST(v.port == "8080");
        BOOST_REQUIRE(v.port_num.has_value());
        BOOST_TEST(*v.port_num == 8080u);
    }
}

BOOST_AUTO_TEST_CASE(Test_EndpointParser_DefaultPort)
{
    {
        auto v = ParseEndpointView("example.net", "2101");
        BOOST_TEST(v.host == "example.net");
        BOOST_TEST(v.port == "2101");
        BOOST_REQUIRE(v.port_num.has_value());
        BOOST_TEST(*v.port_num == 2101u);
    }
    {
        auto v = ParseEndpointView("[::1]", "443");
        BOOST_TEST(v.host == "::1");
        BOOST_TEST(v.port == "443");
        BOOST_REQUIRE(v.port_num.has_value());
        BOOST_TEST(*v.port_num == 443u);
    }
    {
        // Default port overridden by explicit port
        auto v = ParseEndpointView("example.com:9000", "8080");
        BOOST_TEST(v.host == "example.com");
        BOOST_TEST(v.port == "9000");
        BOOST_REQUIRE(v.port_num.has_value());
        BOOST_TEST(*v.port_num == 9000u);
    }
    {
        // Invalid default port
        auto v = ParseEndpointView("example.com", "invalid");
        BOOST_TEST(v.host == "example.com");
        BOOST_TEST(v.port == "invalid");
        BOOST_TEST(!v.port_num.has_value());
    }
    {
        // IPv4 with default port
        auto v = ParseEndpointView("192.168.1.1", "80");
        BOOST_TEST(v.host == "192.168.1.1");
        BOOST_TEST(v.port == "80");
        BOOST_REQUIRE(v.port_num.has_value());
        BOOST_TEST(*v.port_num == 80u);
    }
}

BOOST_AUTO_TEST_CASE(Test_EndpointParser_PortValidation)
{
    {
        // Port 0 should be rejected
        auto v = ParseEndpointView("host:0");
        BOOST_TEST(v.port == "0");
        BOOST_TEST(!v.port_num.has_value()); // Port 0 is invalid
    }
    {
        // Port 1 (minimum valid)
        auto v = ParseEndpointView("host:1");
        BOOST_TEST(v.port == "1");
        BOOST_REQUIRE(v.port_num.has_value());
        BOOST_TEST(*v.port_num == 1u);
    }
    {
        auto v = ParseEndpointView("host:65535");
        BOOST_TEST(v.port == "65535");
        BOOST_REQUIRE(v.port_num.has_value());
        BOOST_TEST(*v.port_num == 65535u);
    }
    {
        auto v = ParseEndpointView("host:65536");
        BOOST_TEST(v.port == "65536");
        BOOST_TEST(!v.port_num.has_value()); // out of range
    }
    {
        auto v = ParseEndpointView("host:notaport");
        BOOST_TEST(v.port == "notaport");
        BOOST_TEST(!v.port_num.has_value()); // non-numeric
    }
    {
        auto v = ParseEndpointView("[::1]:12x3");
        BOOST_TEST(v.port == "12x3");
        BOOST_TEST(!v.port_num.has_value()); // mixed
    }
    {
        // Port too long
        auto v = ParseEndpointView("host:123456");
        BOOST_TEST(v.port == "123456");
        BOOST_TEST(!v.port_num.has_value());
    }
    {
        // Empty port after colon
        auto v = ParseEndpointView("host:");
        BOOST_TEST(v.host == "host");
        BOOST_TEST(v.port.empty());
        BOOST_TEST(!v.port_num.has_value());
    }
    {
        // Leading zeros
        auto v = ParseEndpointView("host:00080");
        BOOST_TEST(v.port == "00080");
        BOOST_REQUIRE(v.port_num.has_value());
        BOOST_TEST(*v.port_num == 80u);
    }
    {
        // Negative port (parsed as non-numeric)
        auto v = ParseEndpointView("host:-80");
        BOOST_TEST(v.port == "-80");
        BOOST_TEST(!v.port_num.has_value());
    }
}

BOOST_AUTO_TEST_CASE(Test_EndpointParser_TrimAndScheme)
{
    {
        auto v = ParseEndpointView("  example.com:80  ");
        BOOST_TEST(v.host == "example.com");
        BOOST_TEST(v.port == "80");
        BOOST_REQUIRE(v.port_num.has_value());
        BOOST_TEST(*v.port_num == 80u);
    }
    {
        auto v = ParseEndpointView("tcp://broker.local:2101");
        BOOST_TEST(v.host == "broker.local");
        BOOST_TEST(v.port == "2101");
        BOOST_REQUIRE(v.port_num.has_value());
        BOOST_TEST(*v.port_num == 2101u);
    }
    {
        auto v = ParseEndpointView("udp://[fe80::1%25en0]:5353");
        BOOST_TEST(v.host == "fe80::1");
        BOOST_TEST(v.scope_id == "en0");
        BOOST_TEST(v.port == "5353");
        BOOST_REQUIRE(v.port_num.has_value());
        BOOST_TEST(*v.port_num == 5353u);
        BOOST_TEST(v.had_brackets);
    }
    {
        // HTTP scheme
        auto v = ParseEndpointView("http://www.example.com:8080");
        BOOST_TEST(v.host == "www.example.com");
        BOOST_TEST(v.port == "8080");
        BOOST_REQUIRE(v.port_num.has_value());
        BOOST_TEST(*v.port_num == 8080u);
    }
    {
        // HTTPS scheme
        auto v = ParseEndpointView("https://secure.example.com");
        BOOST_TEST(v.host == "secure.example.com");
        BOOST_TEST(v.port.empty());
        BOOST_TEST(!v.port_num.has_value());
    }
    {
        // Scheme with IPv4
        auto v = ParseEndpointView("mqtt://192.168.1.100:1883");
        BOOST_TEST(v.host == "192.168.1.100");
        BOOST_TEST(v.port == "1883");
        BOOST_REQUIRE(v.port_num.has_value());
        BOOST_TEST(*v.port_num == 1883u);
    }
    {
        // Mixed whitespace and scheme
        auto v = ParseEndpointView("  \t mqtt://broker.local:1883  \n ");
        BOOST_TEST(v.host == "broker.local");
        BOOST_TEST(v.port == "1883");
        BOOST_REQUIRE(v.port_num.has_value());
        BOOST_TEST(*v.port_num == 1883u);
    }
    {
        // Single character scheme (edge case)
        auto v = ParseEndpointView("x://host:80");
        BOOST_TEST(v.host == "host");
        BOOST_TEST(v.port == "80");
        BOOST_REQUIRE(v.port_num.has_value());
        BOOST_TEST(*v.port_num == 80u);
    }
    {
        // No scheme, just "://" pattern at start (pos=0, not stripped)
        // Multiple colons present, so treated as malformed IPv6 (no split)
        auto v = ParseEndpointView("://host:80");
        BOOST_TEST(v.host == "://host:80");
        BOOST_TEST(v.port.empty());
        BOOST_TEST(!v.port_num.has_value());
    }
}

BOOST_AUTO_TEST_CASE(Test_EndpointParser_TupleAPI)
{
    {
        auto [h, p] = ParseEndpoint("example.com:1234");
        BOOST_TEST(h == "example.com");
        BOOST_TEST(p == "1234");
    }
    {
        auto [h, p] = ParseEndpoint("[2001:db8::42]", "443");
        BOOST_TEST(h == "2001:db8::42");
        BOOST_TEST(p == "443");
    }
    {
        // Empty input
        auto [h, p] = ParseEndpoint("");
        BOOST_TEST(h.empty());
        BOOST_TEST(p.empty());
    }
    {
        // With zone ID
        auto [h, p] = ParseEndpoint("[fe80::1%25eth0]:5353");
        BOOST_TEST(h == "fe80::1");
        BOOST_TEST(p == "5353");
    }
    {
        // Hostname only
        auto [h, p] = ParseEndpoint("localhost");
        BOOST_TEST(h == "localhost");
        BOOST_TEST(p.empty());
    }
}

BOOST_AUTO_TEST_CASE(Test_EndpointParser_EdgeCases)
{
    {
        // Just a colon
        auto v = ParseEndpointView(":");
        BOOST_TEST(v.host.empty());
        BOOST_TEST(v.port.empty());
        BOOST_TEST(!v.port_num.has_value());
    }
    {
        // Multiple colons in hostname (should not split)
        auto v = ParseEndpointView("a:b:c:d");
        BOOST_TEST(v.host == "a:b:c:d");
        BOOST_TEST(v.port.empty());
        BOOST_TEST(!v.port_num.has_value());
    }
    {
        // Bracket with colon but no port number
        auto v = ParseEndpointView("[::1]:");
        BOOST_TEST(v.had_brackets);
        BOOST_TEST(v.host == "::1");
        BOOST_TEST(v.port.empty());
        BOOST_TEST(!v.port_num.has_value());
    }
    {
        // IPv6 with zone but malformed percent encoding
        auto v = ParseEndpointView("[fe80::1%eth0]:80");
        BOOST_TEST(v.had_brackets);
        BOOST_TEST(v.host == "fe80::1");
        BOOST_TEST(v.scope_id == "eth0"); // %eth0 -> eth0 (not percent-encoded)
        BOOST_TEST(v.port == "80");
    }
    {
        // Very long hostname
        std::string long_host(200, 'a');
        long_host += ":8080";
        auto v = ParseEndpointView(long_host);
        BOOST_TEST(v.host.size() == 200u);
        BOOST_TEST(v.port == "8080");
    }
    {
        // Port at max length (5 digits)
        auto v = ParseEndpointView("host:12345");
        BOOST_TEST(v.port == "12345");
        BOOST_REQUIRE(v.port_num.has_value());
        BOOST_TEST(*v.port_num == 12345u);
    }
    {
        // Scheme without host
        auto v = ParseEndpointView("http://");
        BOOST_TEST(v.host.empty());
        BOOST_TEST(v.port.empty());
        BOOST_TEST(!v.port_num.has_value());
    }
    {
        // Scheme with just port
        auto v = ParseEndpointView("http://:8080");
        BOOST_TEST(v.host.empty());
        BOOST_TEST(v.port == "8080");
        BOOST_REQUIRE(v.port_num.has_value());
        BOOST_TEST(*v.port_num == 8080u);
    }
}

BOOST_AUTO_TEST_CASE(Test_EndpointParser_SpecialCharacters)
{
    {
        // Underscore in hostname (valid in DNS)
        auto v = ParseEndpointView("my_server.local:80");
        BOOST_TEST(v.host == "my_server.local");
        BOOST_TEST(v.port == "80");
    }
    {
        // Dots in hostname
        auto v = ParseEndpointView("a.b.c.d.e.f:443");
        BOOST_TEST(v.host == "a.b.c.d.e.f");
        BOOST_TEST(v.port == "443");
    }
    {
        // Hostname ending with dot (absolute DNS name)
        auto v = ParseEndpointView("example.com.:80");
        BOOST_TEST(v.host == "example.com.");
        BOOST_TEST(v.port == "80");
    }
}

BOOST_AUTO_TEST_CASE(Test_EndpointParser_ZoneIdVariations)
{
    {
        // Zone ID with just "25" (becomes empty after decode)
        auto v = ParseEndpointView("[fe80::1%25]:80");
        BOOST_TEST(v.had_brackets);
        BOOST_TEST(v.host == "fe80::1");
        BOOST_TEST(v.scope_id.empty());
        BOOST_TEST(v.port == "80");
    }
    {
        // Zone ID starting with 25 but more content
        auto v = ParseEndpointView("[fe80::1%2525]:80");
        BOOST_TEST(v.had_brackets);
        BOOST_TEST(v.host == "fe80::1");
        BOOST_TEST(v.scope_id == "25");
        BOOST_TEST(v.port == "80");
    }
    {
        // Zone ID not starting with 25
        auto v = ParseEndpointView("[fe80::1%24]:80");
        BOOST_TEST(v.had_brackets);
        BOOST_TEST(v.host == "fe80::1");
        BOOST_TEST(v.scope_id == "24");
        BOOST_TEST(v.port == "80");
    }
    {
        // Zone ID with alphanumeric
        auto v = ParseEndpointView("[fe80::1%25en0-wifi]:80");
        BOOST_TEST(v.had_brackets);
        BOOST_TEST(v.host == "fe80::1");
        BOOST_TEST(v.scope_id == "en0-wifi");
        BOOST_TEST(v.port == "80");
    }
}

BOOST_AUTO_TEST_SUITE_END();
