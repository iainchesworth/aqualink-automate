#include <cstdint>
#include <filesystem>
#include <format>
#include <optional>
#include <stdexcept>
#include <string>

#include <Windows.h>
#include <atlbase.h>
#include <comdef.h>
#include <comutil.h>
#include <initguid.h>
#include <Netfw.h>

#include <boost/dll/runtime_symbol_info.hpp>
#include <boost/lexical_cast.hpp>

#include "developer/firewall_manager.h"
#include "logging/logging.h"
#include "platform/windows/uac_elevation.h"
#include "version/version.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Developer::FirewallUtils
{

	using ComRulePtr = CComPtr<INetFwRule>;
	using ComRulesPtr = CComPtr<INetFwRules>;
	using ComPolicyPtr = CComPtr<INetFwPolicy2>;

    std::optional<ComPolicyPtr> InitializePolicy()
    {
        ComPolicyPtr policy;
      
        if (FAILED(::CoCreateInstance(__uuidof(NetFwPolicy2), nullptr, CLSCTX_INPROC_SERVER, __uuidof(INetFwPolicy2), reinterpret_cast<void**>(&policy))))
        {
            return std::nullopt;
        }
        
        return policy;
    }

    std::optional<ComRulesPtr> GetRules(const ComPolicyPtr& policy)
    {
        ComRulesPtr rules;

        if (FAILED(policy->get_Rules(&rules)))
        {
            return std::nullopt;
        }

        return rules;
    }

    std::optional<ComRulePtr> FindRule(const ComRulesPtr& rules, const std::string& rule_name)
    {
        ComRulePtr pNetFwRule;

        if (FAILED(rules->Item(_bstr_t(rule_name.c_str()), &pNetFwRule)))
        {
            return std::nullopt;
        }

        return pNetFwRule;
    }

    std::optional<std::filesystem::path> GetCurrentApplicationPath()
    {
        try
        {
            return std::filesystem::path(std::filesystem::absolute(boost::dll::program_location().string()));
        }
        catch (const std::bad_alloc& ex_ba)
        {
            return std::nullopt;
        }
        catch (const boost::dll::fs::system_error& ex_bds_se)
        {
            return std::nullopt;
        }
    }

	void CheckAndConfigureExceptions()
	{
        if (auto com_init = ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED); RPC_E_CHANGED_MODE != com_init && FAILED(com_init))
        {
            LogDebug(Channel::Platform, std::format("Firewall utilities failed to initialise COM; error was: {}", ::_com_error(com_init).ErrorMessage()));
        }
        else if (auto application_is_elevated = IsElevated(); !application_is_elevated.value_or(false))
        {
            LogDebug(Channel::Platform, "Application is not elevated (or failed to get status); cannot configure firewall exceptions");
        }
        else if (auto policy = InitializePolicy(); !policy)
        {
            LogWarning(Channel::Platform, "Firewall utilities failed to initialise firewall policy");
        }
        else if (auto rules = GetRules(*policy); !rules)
        {
            LogWarning(Channel::Platform, "Firewall utilities failed to retrieve current firewall rules");
        }
        else if (auto app_path = GetCurrentApplicationPath(); !app_path)
        {
            LogWarning(Channel::Platform, "Firewall utilities failed to retrieve path to executable (for exception)");
        }
        else
        {
            //
            // Firewall String Formats
            //
            //   Rule Name:     <Name of the feature> - <Subfeature> (<Protocol>-<Dir>-<Counter>)
            //   Rule Desc:     <Dir>bound rule for <Name of the feature> to allow ... [<UDP/TCP> <Port>]
            //   Group Name:    <Name of the feature>
            //
            //
            // Specific Firewall Rules (Group -> Name -> Desc)
            //
            //    "AqualinkAutomate" -> "AqualinkAutomate - Web Server (HTTP-In)"   -> "Inbound rule for AqualinkAutomate to allow access to the admin portal [TCP 80]"
            //    "AqualinkAutomate" -> "AqualinkAutomate - Web Server (HTTPS-In)"  -> "Inbound rule for AqualinkAutomate to allow secure access to the admin portal [TCP 443]"
            //
            //    "AqualinkAutomate" -> "AqualinkAutomate - MQTT Client (MQTT-Out)" -> "Outbound rule for AqualinkAutomate to allow connections to remote MQTT brokers [TCP 1883]"
            //    "AqualinkAutomate" -> "AqualinkAutomate - MQTT Client (MQTTS-Out)" -> "Outbound rule for AqualinkAutomate to allow secure connections to remote MQTT brokers [TCP 8883]"
            //
            // Security scoping
            //
            //   Inbound (admin portal) rules are scoped so they cannot silently expose the
            //   machine on hostile networks when the process happens to be run elevated:
            //     - Profiles are restricted to PRIVATE | DOMAIN (the PUBLIC profile, e.g. an
            //       airport / coffee-shop Wi-Fi, is deliberately excluded).
            //     - The remote-address scope is restricted to "LocalSubnet" so only hosts on
            //       the same network segment may reach the admin portal.
            //   Outbound (MQTT client) rules keep an unrestricted remote-address scope because
            //   the broker may legitimately live anywhere, but are likewise kept off the PUBLIC
            //   profile.
            //
            //   NOTE (cross-unit): the inbound port numbers below remain the historical
            //   defaults (HTTP 80 / HTTPS 443). Wiring the actually-configured
            //   --http-port/--https-port (Options::Web::WebSettings) into these rules requires
            //   threading the settings through CheckAndConfigureExceptions(), which touches the
            //   shared firewall_manager.h declaration and the aqualink-automate.cpp call site
            //   (owned by other units). The literals are lifted to named constants here so that
            //   change is a single substitution once the signature is widened.
            //

            static const std::string APP_NAME{ Version::VersionInfo::ProjectName() };

            // Restrict auto-created rules away from the PUBLIC profile so an elevated run on an
            // untrusted network does not open the admin portal to the world.
            static constexpr long SCOPED_PROFILES{ NET_FW_PROFILE2_PRIVATE | NET_FW_PROFILE2_DOMAIN };

            // Remote-address scopes (BSTR-compatible literals understood by INetFwRule).
            static constexpr wchar_t REMOTE_LOCAL_SUBNET[]{ L"LocalSubnet" };
            static constexpr wchar_t REMOTE_ANY[]{ L"*" };

            static constexpr uint16_t HTTP_PORT{ 80 };
            static constexpr uint16_t HTTPS_PORT{ 443 };
            static constexpr uint16_t MQTT_PORT{ 1883 };
            static constexpr uint16_t MQTTS_PORT{ 8883 };

            using FirewallRule = std::tuple<std::string, std::string, decltype(NET_FW_IP_PROTOCOL_TCP), uint16_t, decltype(NET_FW_RULE_DIR_IN), const wchar_t*>;
            using FirewallRuleCollection = std::vector<FirewallRule>;
            using FirewallGroup = std::tuple<std::string, std::filesystem::path, FirewallRuleCollection>;

            FirewallGroup firewall_group
            {
                APP_NAME,
                *app_path,
                {
                    {
                        std::format("{} - Web Server (HTTP-In)", APP_NAME),
                        std::format("Inbound rule for {} to allow access to the admin portal [TCP {}]", APP_NAME, HTTP_PORT),
                        NET_FW_IP_PROTOCOL_TCP,
                        HTTP_PORT,
                        NET_FW_RULE_DIR_IN,
                        REMOTE_LOCAL_SUBNET
                    },
                    {
                        std::format("{} - Web Server (HTTPS-In)", APP_NAME),
                        std::format("Inbound rule for {} to allow secure access to the admin portal [TCP {}]", APP_NAME, HTTPS_PORT),
                        NET_FW_IP_PROTOCOL_TCP,
                        HTTPS_PORT,
                        NET_FW_RULE_DIR_IN,
                        REMOTE_LOCAL_SUBNET
                    },
                    {
                        std::format("{} - MQTT Client (MQTT-Out)", APP_NAME),
                        std::format("Outbound rule for {} to allow connections to remote MQTT brokers [TCP {}]", APP_NAME, MQTT_PORT),
                        NET_FW_IP_PROTOCOL_TCP,
                        MQTT_PORT,
                        NET_FW_RULE_DIR_OUT,
                        REMOTE_ANY
                    },
                    {
                        std::format("{} - MQTT Client (MQTTS-Out)", APP_NAME),
                        std::format("Outbound rule for {} to allow secure connections to remote MQTT brokers [TCP {}]", APP_NAME, MQTTS_PORT),
                        NET_FW_IP_PROTOCOL_TCP,
                        MQTTS_PORT,
                        NET_FW_RULE_DIR_OUT,
                        REMOTE_ANY
                    }
                }
            };

            auto add_new_firewall_rule = [](auto& rules, auto& name, auto& desc, auto& app, auto& proto, auto& port, auto& direction, const wchar_t* remote_addresses) -> void
                {
                    ComRulePtr pNetFwRule;

                    if (auto hr = ::CoCreateInstance(__uuidof(NetFwRule), nullptr, CLSCTX_INPROC_SERVER, __uuidof(INetFwRule), reinterpret_cast<void**>(&pNetFwRule)); FAILED(hr))
                    {
                        LogWarning(Channel::Platform, std::format("Firewall utilities failed to instantiate rule; error was -> {}", ::_com_error(hr).ErrorMessage()));
                    }
                    else
                    {
                        auto rule_app = _bstr_t(app.c_str());
                        auto rule_name = _bstr_t(name.c_str());
                        auto rule_desc = _bstr_t(desc.c_str());
                        auto rule_ports = _bstr_t(std::format("{}", port).c_str());
                        auto rule_remote = _bstr_t(remote_addresses);

                        pNetFwRule->put_Name(rule_name);
                        pNetFwRule->put_Description(rule_desc);
                        pNetFwRule->put_ApplicationName(rule_app);
                        pNetFwRule->put_Protocol(proto);
                        pNetFwRule->put_LocalPorts(rule_ports);
                        pNetFwRule->put_RemoteAddresses(rule_remote);
                        pNetFwRule->put_Action(NET_FW_ACTION_ALLOW);
                        pNetFwRule->put_Direction(direction);
                        pNetFwRule->put_Enabled(VARIANT_TRUE);
                        pNetFwRule->put_Profiles(SCOPED_PROFILES);

                        if (auto hr = rules->Add(pNetFwRule); FAILED(hr))
                        {
                            LogWarning(Channel::Platform, std::format("Firewall utilities failed to add firewall exception for application; error was -> {}", ::_com_error(hr).ErrorMessage()));
                        }
                        else
                        {
                            LogTrace(Channel::Platform, "Firewall utilities added application exception successfully");
                        }
                    }
                };

            for (auto& [name, desc, proto, port, direction, remote_addresses] : std::get<FirewallRuleCollection>(firewall_group))
            {
                if (auto rule = FindRule(*rules, name); !rule)
                {
                    add_new_firewall_rule(*rules, name, desc, std::get<std::filesystem::path>(firewall_group), proto, port, direction, remote_addresses);
                }
            }
        }

        ::CoUninitialize();
	}

}
// namespace AqualinkAutomate::Developer::FirewallUtils
