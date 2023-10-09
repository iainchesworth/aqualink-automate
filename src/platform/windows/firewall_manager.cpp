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
#include <tl/expected.hpp>

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
      
        if (FAILED(::CoCreateInstance(__uuidof(NetFwPolicy2), NULL, CLSCTX_INPROC_SERVER, __uuidof(INetFwPolicy2), (void**)&policy)))
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
        if (auto com_init = ::CoInitializeEx(0, COINIT_APARTMENTTHREADED); RPC_E_CHANGED_MODE != com_init && FAILED(com_init))
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

            static const std::string APP_NAME{ Version::VersionInfo::ProjectName() };

            using FirewallRule = std::tuple<std::string, std::string, decltype(NET_FW_IP_PROTOCOL_TCP), uint16_t, decltype(NET_FW_RULE_DIR_IN)>;
            using FirewallRuleCollection = std::vector<FirewallRule>;
            using FirewallGroup = std::tuple<std::string, std::filesystem::path, FirewallRuleCollection>;

            FirewallGroup firewall_group
            {
                APP_NAME,
                *app_path,
                {
                    { 
                        std::format("{} - Web Server (HTTP-In)", APP_NAME),
                        std::format("Inbound rule for {} to allow access to the admin portal [TCP {}]", APP_NAME, 80),
                        NET_FW_IP_PROTOCOL_TCP, 
                        80, 
                        NET_FW_RULE_DIR_IN 
                    },
                    {
                        std::format("{} - Web Server (HTTPS-In)", APP_NAME),
                        std::format("Inbound rule for {} to allow secure access to the admin portal [TCP {}]", APP_NAME, 443),
                        NET_FW_IP_PROTOCOL_TCP, 
                        443, 
                        NET_FW_RULE_DIR_IN 
                    },
                    {
                        std::format("{} - MQTT Client (MQTT-Out)", APP_NAME),
                        std::format("Outbound rule for {} to allow connections to remote MQTT brokers [TCP {}]", APP_NAME, 1883),
                        NET_FW_IP_PROTOCOL_TCP, 
                        1883, 
                        NET_FW_RULE_DIR_OUT 
                    },
                    {
                        std::format("{} - MQTT Client (MQTTS-Out)", APP_NAME),
                        std::format("Outbound rule for {} to allow secure connections to remote MQTT brokers [TCP {}]", APP_NAME, 8883),
                        NET_FW_IP_PROTOCOL_TCP, 
                        8883, 
                        NET_FW_RULE_DIR_OUT 
                    }
                }
            };

            auto add_new_firewall_rule = [](auto& rules, auto& name, auto& desc, auto& app, auto& proto, auto& port, auto& direction) -> void
                {
                    ComRulePtr pNetFwRule;

                    if (auto hr = ::CoCreateInstance(__uuidof(NetFwRule), NULL, CLSCTX_INPROC_SERVER, __uuidof(INetFwRule), (void**)&pNetFwRule); FAILED(hr))
                    {
                        LogWarning(Channel::Platform, std::format("Firewall utilities failed to instantiate rule; error was -> {}", ::_com_error(hr).ErrorMessage()));
                    }
                    else
                    {
                        auto rule_app = _bstr_t(app.c_str());
                        auto rule_name = _bstr_t(name.c_str());
                        auto rule_desc = _bstr_t(desc.c_str());
                        auto rule_ports = _bstr_t(std::format("{}", port).c_str());

                        pNetFwRule->put_Name(rule_name);
                        pNetFwRule->put_Description(rule_desc);
                        pNetFwRule->put_ApplicationName(rule_app);
                        pNetFwRule->put_Protocol(proto);
                        pNetFwRule->put_LocalPorts(rule_ports);
                        pNetFwRule->put_Action(NET_FW_ACTION_ALLOW);
                        pNetFwRule->put_Direction(direction);
                        pNetFwRule->put_Enabled(VARIANT_TRUE);
                        pNetFwRule->put_Profiles(NET_FW_PROFILE2_ALL);

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

            for (auto& [name, desc, proto, port, direction] : std::get<FirewallRuleCollection>(firewall_group))
            {
                if (auto rule = FindRule(*rules, name); !rule)
                {
                    add_new_firewall_rule(*rules, name, desc, std::get<std::filesystem::path>(firewall_group), proto, port, direction);
                }
            }
        }

        ::CoUninitialize();
	}

}
// namespace AqualinkAutomate::Developer::FirewallUtils
