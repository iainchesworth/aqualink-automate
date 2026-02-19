# WebSocket Protocol Reference

## Message Contract

- All messages use the envelope: `{ "type": "string", "payload": {} }`
- Message types defined as a C++ `enum class` in `src/ws/messages.hpp`.
- Mirrored as JS constants in `web/js/message-types.js`.
- **Both sides must always be updated together.** After any change, run the build and E2E tests.
- Use versioned schemas if the protocol evolves: add `"v": 1` to the envelope.
- Document every message type with its payload shape and direction (server→client, client→server, bidirectional) in `PROTOCOL.md`.

## Example PROTOCOL.md Entry

```
## game:state_update (server → client)
Pushed when authoritative game state changes.

Payload:
{
  "round_id": "string",
  "status": "betting" | "in_progress" | "resolved",
  "balance": number,
  "timestamp": number (unix ms),
  "seq": number
}

Notes:
- Client must process in sequence order; discard messages with seq <= last processed.
- Balance is display-only; not authoritative for bet validation.
```

## Connection Management — C++ Server Side

- Associate each WebSocket connection with an authenticated session at handshake.
- Reject unauthenticated frames immediately.
- Implement per-connection send queues with backpressure — never block the event loop.
- Use ping/pong frames for heartbeat; configure a timeout (e.g., 30s with no pong → close).
- On disconnect:
  - Cancel all pending timers and scheduled operations for the session.
  - Remove the session from all subscription/broadcast groups.
  - Release any held resources (locks, reserved slots).
  - Log the disconnect with session ID, duration, and close code.
- Use text frames with JSON by default. Switch to binary (MessagePack, CBOR) only if profiling shows JSON serialisation is a bottleneck.
- Assign a correlation ID to each session; include it in all log entries for that connection.

## Connection Management — JS Client Side

- Manage the WebSocket in a dedicated Alpine.js store (`Alpine.store('connection')`).
- Never store the WebSocket instance as a bare global variable.
- Implement automatic reconnection with exponential backoff:
  - Initial delay: 1s, multiplier: 2x, max delay: 30s, with jitter.
  - Queue outbound messages during reconnection; flush on successful reconnect.
- Show a visible connection-status indicator in the UI:
  - Connected (green), Reconnecting (amber/spinner), Disconnected (red).
  - Disable interactive elements while disconnected.
- Handle server-initiated close codes:
  - 1000 (normal) — clean disconnect.
  - 4001 (session expired) — clear local state, redirect to login.
  - 4002 (rate limited) — show message, do not attempt immediate reconnect.
  - 4003 (maintenance) — show maintenance message.
- Use a single WebSocket connection per session; multiplex all message types over it.
- Parse incoming messages in the connection store; dispatch to domain stores by message type.

## Sequence / Ordering

- Include a monotonically increasing `seq` field in server→client messages.
- Client tracks `lastSeq` per channel/topic; discard messages with `seq <= lastSeq`.
- On reconnect, the client sends its `lastSeq`; the server replays missed messages or sends a full state snapshot.
- For time-sensitive data (live odds, countdowns), include a `timestamp` field; the client can detect and compensate for latency.
