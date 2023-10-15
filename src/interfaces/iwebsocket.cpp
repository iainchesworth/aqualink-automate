#include <format>

#include "http/server/websocket_plainsession.h"
#include "http/server/websocket_sslsession.h"
#include "interfaces/iwebsocket.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Interfaces
{
    
    void IWebSocketBase::Handle_OnOpen(std::shared_ptr<Interfaces::ISession> session)
    {
        if (nullptr == session)
        {
            LogDebug(Channel::Web, "Invalid session (nullptr); cannot action OnOpen handler");
        }
        else
        {
            m_ActiveSessions.emplace(session);
            OnOpen();
        }
    }

    void IWebSocketBase::Handle_OnMessage(std::shared_ptr<Interfaces::ISession> session, const boost::beast::flat_buffer& buffer)
    {
        if (nullptr == session)
        {
            LogDebug(Channel::Web, "Invalid session (nullptr); cannot action OnMessage handler");
        }
        else
        {
            OnMessage(buffer);
        }
    }

    void IWebSocketBase::Handle_OnClose(std::shared_ptr<Interfaces::ISession> session)
    {
        if (nullptr == session)
        {
            LogDebug(Channel::Web, "Invalid session (nullptr); cannot action OnClose handler");
        }
        else
        {
            if (auto session_it = m_ActiveSessions.find(session); m_ActiveSessions.end() != session_it)
            {
                m_ActiveSessions.erase(session_it);
            }

            OnClose();
        }
    }

    void IWebSocketBase::Handle_OnError(std::shared_ptr<Interfaces::ISession> session)
    {
        if (nullptr == session)
        {
            LogDebug(Channel::Web, "Invalid session (nullptr); cannot action OnError handler");
        }
        else
        {
            if (auto session_it = m_ActiveSessions.find(session); m_ActiveSessions.end() != session_it)
            {
                m_ActiveSessions.erase(session_it);
            }

            OnError();
        }
    }

    void IWebSocketBase::BroadcastMessage_AsBinary(const std::string& buffer)
    {
        LogTrace(Channel::Web, std::format("Broadcasting binary message to all sessions; message -> {}", buffer));

        auto buffer_ptr = std::make_shared<const std::string>(buffer);

        std::for_each(std::execution::par, m_ActiveSessions.cbegin(), m_ActiveSessions.cend(),
            [this, buffer_ptr](const auto& session) -> void
            {
                LogTrace(Channel::Web, std::format("Broadcasting binary message to session"));
                PublishMessage_AsBinary(session, buffer_ptr);
            }
        );
    }

    void IWebSocketBase::BroadcastMessage_AsText(const std::string& buffer)
    {
        LogTrace(Channel::Web, std::format("Broadcasting text message to all sessions; message -> {}", buffer));

        auto buffer_ptr = std::make_shared<const std::string>(buffer);

        std::for_each(std::execution::par, m_ActiveSessions.cbegin(), m_ActiveSessions.cend(),
            [this, buffer_ptr](const auto& session) -> void
            {
                LogTrace(Channel::Web, std::format("Broadcasting text message to session"));
                PublishMessage_AsText(session, buffer_ptr);
            }
        );
    }

    void IWebSocketBase::PublishMessage_AsBinary(std::shared_ptr<Interfaces::ISession> session, std::shared_ptr<const std::string> buffer_ptr)
    {
        if (nullptr == session)
        {
            LogTrace(Channel::Web, "Attempted to publish a binary message to an invalid session (nullptr)");
        }
        else if (auto session_ptr = std::dynamic_pointer_cast<HTTP::WebSocket_PlainSession>(session); nullptr != session_ptr)
        {
            session_ptr->QueueWrite(buffer_ptr, true);
        }
        else if (auto session_ptr = std::dynamic_pointer_cast<HTTP::WebSocket_SSLSession>(session); nullptr != session_ptr)
        {
            session_ptr->QueueWrite(buffer_ptr, true);
        }
        else
        {
            LogDebug(Channel::Web, "Failed to find a valid WebSocket session; could not sent binary message");
        }
    }

    void IWebSocketBase::PublishMessage_AsText(std::shared_ptr<Interfaces::ISession> session, std::shared_ptr<const std::string> buffer_ptr)
    {
        if (nullptr == session)
        {
            LogTrace(Channel::Web, "Attempted to publish a text message to an invalid session (nullptr)");
        }
        else if (auto session_ptr = std::dynamic_pointer_cast<HTTP::WebSocket_PlainSession>(session); nullptr != session_ptr)
        {
            session_ptr->QueueWrite(buffer_ptr, false);
        }
        else if (auto session_ptr = std::dynamic_pointer_cast<HTTP::WebSocket_SSLSession>(session); nullptr != session_ptr)
        {
            session_ptr->QueueWrite(buffer_ptr, false);
        }
        else
        {
            LogDebug(Channel::Web, "Failed to find a valid WebSocket session; could not sent text message");
        }
    }

}
// namespace AqualinkAutomate::Interfaces
