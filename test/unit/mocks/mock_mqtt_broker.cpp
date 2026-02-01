#include "mock_mqtt_broker.h"

#include <algorithm>
#include <regex>

using namespace std::string_literals;
using namespace std::chrono_literals;

namespace AqualinkAutomate::Test::Mocks
{
	/*
	MockMqttBroker::MockMqttBroker(boost::asio::io_context& ioc)
		: m_ioc(ioc)
	{
	}

	boost::cobalt::task<void> MockMqttBroker::start(uint16_t port)
	{
		m_port = port;
		m_running = true;
		co_return;
	}

	boost::cobalt::task<void> MockMqttBroker::stop()
	{
		m_running = false;
		m_clients.clear();
		co_return;
	}

	bool MockMqttBroker::is_running() const
	{
		return m_running;
	}

	void MockMqttBroker::set_authentication_required(bool required)
	{
		m_auth_required = required;
	}

	void MockMqttBroker::add_user(const std::string& username, const std::string& password)
	{
		m_users[username] = password;
	}

	void MockMqttBroker::set_connection_should_fail(bool fail)
	{
		m_connection_should_fail = fail;
	}

	void MockMqttBroker::set_publish_should_fail(bool fail)
	{
		m_publish_should_fail = fail;
	}

	void MockMqttBroker::set_subscribe_should_fail(bool fail)
	{
		m_subscribe_should_fail = fail;
	}

	boost::cobalt::task<void> MockMqttBroker::inject_message(const std::string& topic, const std::string& payload, 
		uint8_t qos, bool retain)
	{
		MockMqttMessage message(topic, payload, qos, retain);
		
		// Store the message
		m_published_messages.push_back(message);
		
		// Deliver to subscribed clients
		for (const auto& [client_id, client] : m_clients)
		{
			for (const auto& subscription : client->subscriptions)
			{
				if (topic_matches_filter(topic, subscription))
				{
					co_await publish_to_client(client_id, message);
					break;
				}
			}
		}
		
		// Call message handler if set
		if (m_message_handler)
		{
			m_message_handler(message);
		}
	}

	std::vector<std::string> MockMqttBroker::get_connected_clients() const
	{
		std::vector<std::string> clients;
		for (const auto& [client_id, client] : m_clients)
		{
			clients.push_back(client_id);
		}
		return clients;
	}

	std::vector<MockMqttMessage> MockMqttBroker::get_published_messages() const
	{
		return m_published_messages;
	}

	std::vector<std::string> MockMqttBroker::get_subscriptions(const std::string& client_id) const
	{
		auto it = m_clients.find(client_id);
		if (it != m_clients.end())
		{
			return it->second->subscriptions;
		}
		return {};
	}

	void MockMqttBroker::clear_published_messages()
	{
		m_published_messages.clear();
	}

	void MockMqttBroker::set_connect_handler(ConnectHandler handler)
	{
		m_connect_handler = std::move(handler);
	}

	void MockMqttBroker::set_disconnect_handler(DisconnectHandler handler)
	{
		m_disconnect_handler = std::move(handler);
	}

	void MockMqttBroker::set_message_handler(MessageHandler handler)
	{
		m_message_handler = std::move(handler);
	}

	void MockMqttBroker::set_subscribe_handler(SubscribeHandler handler)
	{
		m_subscribe_handler = std::move(handler);
	}

	boost::cobalt::task<std::shared_ptr<MockMqttBroker::MockClientConnection>> 
		MockMqttBroker::accept_connection(const std::string& client_id, const std::string& username, const std::string& password)
	{
		if (m_connection_should_fail)
		{
			if (m_connect_handler)
			{
				m_connect_handler(client_id, false);
			}
			throw std::runtime_error("Mock broker: Connection refused");
		}

		if (m_auth_required && !authenticate_user(username, password))
		{
			if (m_connect_handler)
			{
				m_connect_handler(client_id, false);
			}
			throw std::runtime_error("Mock broker: Authentication failed");
		}

		auto client = std::make_shared<MockClientConnection>();
		client->client_id = client_id;
		client->authenticated = true;

		m_clients[client_id] = client;

		if (m_connect_handler)
		{
			m_connect_handler(client_id, true);
		}

		co_return client;
	}

	boost::cobalt::task<void> MockMqttBroker::disconnect_client(const std::string& client_id)
	{
		auto it = m_clients.find(client_id);
		if (it != m_clients.end())
		{
			m_clients.erase(it);
			
			if (m_disconnect_handler)
			{
				m_disconnect_handler(client_id);
			}
		}
		co_return;
	}

	boost::cobalt::task<void> MockMqttBroker::publish_to_client(const std::string& client_id, const MockMqttMessage& message)
	{
		if (m_publish_should_fail)
		{
			throw std::runtime_error("Mock broker: Publish failed");
		}

		auto it = m_clients.find(client_id);
		if (it != m_clients.end())
		{
			// Simulate async delivery
			boost::asio::steady_timer timer(m_ioc);
			timer.expires_after(1ms);
			co_await timer.async_wait(boost::asio::use_awaitable);
			
			it->second->message_signal(message);
		}
		co_return;
	}

	boost::cobalt::task<void> MockMqttBroker::subscribe_client(const std::string& client_id, const std::string& topic_filter)
	{
		if (m_subscribe_should_fail)
		{
			throw std::runtime_error("Mock broker: Subscribe failed");
		}

		auto it = m_clients.find(client_id);
		if (it != m_clients.end())
		{
			it->second->subscriptions.push_back(topic_filter);
			
			if (m_subscribe_handler)
			{
				m_subscribe_handler(client_id, topic_filter);
			}
		}
		co_return;
	}

	bool MockMqttBroker::authenticate_user(const std::string& username, const std::string& password) const
	{
		auto it = m_users.find(username);
		return it != m_users.end() && it->second == password;
	}

	bool MockMqttBroker::topic_matches_filter(const std::string& topic, const std::string& filter) const
	{
		if (filter == topic)
		{
			return true;
		}

		// Simple wildcard matching for MQTT topics
		// + matches single level, # matches multiple levels
		std::string regex_pattern = filter;
		
		// Escape special regex characters except our wildcards
		regex_pattern = std::regex_replace(regex_pattern, std::regex(R"([\.\^\$\|\(\)\[\]\*\+\?\{\}\\])"), R"(\$&)");
		
		// Convert MQTT wildcards to regex
		regex_pattern = std::regex_replace(regex_pattern, std::regex(R"(\\\+)"), "[^/]+");  // + -> single level
		regex_pattern = std::regex_replace(regex_pattern, std::regex(R"(\\\#)"), ".*");     // # -> multi level
		
		try
		{
			std::regex topic_regex("^" + regex_pattern + "$");
			return std::regex_match(topic, topic_regex);
		}
		catch (const std::regex_error&)
		{
			// If regex compilation fails, fall back to exact match
			return filter == topic;
		}
	}
	*/
}
// namespace AqualinkAutomate::Test::Mocks