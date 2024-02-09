#include <execution>
#include <format>

#include "http/server/websocket_plainsession.h"
#include "http/server/websocket_sslsession.h"
#include "interfaces/iwebsocket.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Interfaces
{
    IWebSocketBase::~IWebSocketBase()
    {
        if (m_ActiveSessions.empty())
        {
            // No active sessions...do nothing.
        }
        else
        {
            LogDebug(Channel::Web, std::format("Destroying {} active websocket sessions...", m_ActiveSessions.size()));
        
            for (const auto& session : m_ActiveSessions)
            {
                LogTrace(Channel::Web, std::format("Websocket id {} has a use_count of {}", boost::uuids::to_string(session->Id()), session.use_count()));

                session->Stop();
            }

            m_ActiveSessions.clear();
        }
    }
    
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
            OnClose();

            if (auto session_it = m_ActiveSessions.find(session); m_ActiveSessions.end() != session_it)
            {
                m_ActiveSessions.erase(session_it);
            }
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
            OnError();

            if (auto session_it = m_ActiveSessions.find(session); m_ActiveSessions.end() != session_it)
            {
                m_ActiveSessions.erase(session_it);
            }
        }
    }

    void IWebSocketBase::BroadcastMessage(const std::vector<uint8_t>& buffer)
    {
        LogTrace(Channel::Web, std::format("Broadcasting binary message to all ({}) sessions; message -> <is binary data>", m_ActiveSessions.size()));

        std::for_each(std::execution::par, m_ActiveSessions.cbegin(), m_ActiveSessions.cend(),
            [this, &buffer](const auto& session) -> void
            {
                LogTrace(Channel::Web, std::format("Broadcasting binary message ({} bytes) to session {}", buffer.size(), boost::uuids::to_string(session->Id())));
                PublishMessage(session, buffer);
            }
        );
    }

    void IWebSocketBase::BroadcastMessage(const std::string& buffer)
    {
        LogTrace(Channel::Web, std::format("Broadcasting text message (copy) to all ({}) sessions; message -> {}", m_ActiveSessions.size(), buffer));

        std::for_each(std::execution::par, m_ActiveSessions.cbegin(), m_ActiveSessions.cend(),
            [this, &buffer](const auto& session) -> void
            {
                LogTrace(Channel::Web, std::format("Broadcasting text message ({} bytes) to session {}", buffer.size(), boost::uuids::to_string(session->Id())));
                PublishMessage(session, buffer);
            }
        );
    }

    void IWebSocketBase::PublishMessage(std::shared_ptr<Interfaces::ISession> session, std::vector<uint8_t> buffer)
    {
        if (nullptr == session)
        {
            LogTrace(Channel::Web, "Attempted to publish a binary message to an invalid session (nullptr)");
        }
        else
        {
            LogTrace(Channel::Web, std::format("Queuing binary message to session {}", boost::uuids::to_string(session->Id())));
            session->QueueWrite(std::move(buffer));
        }
    }

    void IWebSocketBase::PublishMessage(std::shared_ptr<Interfaces::ISession> session, std::string buffer)
    {
        if (nullptr == session)
        {
            LogTrace(Channel::Web, "Attempted to publish a text message to an invalid session (nullptr)");
        }
        else
        {
            LogTrace(Channel::Web, std::format("Queuing text message to session {}", boost::uuids::to_string(session->Id())));
            session->QueueWrite(std::move(buffer));
        }
    }

}
// namespace AqualinkAutomate::Interfaces
