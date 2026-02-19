# Security Reference

## Server-Side (C++)

- The C++ backend is the **sole authority** for game state and monetary calculations. No client-supplied computed values are trusted.
- Validate and sanitise ALL input: REST body, query params, WebSocket message payloads. Check types, ranges, sizes, and formats.
- Authenticate WebSocket connections during the handshake (token in cookie, query param, or first message).
- Authorise every action per-session. Never trust client-supplied user or session identifiers in message payloads.
- Rate-limit API requests and WebSocket messages per session and per IP.
- Use parameterised queries for all database access. Never interpolate strings into SQL.
- Log security-relevant events with correlation IDs:
  - Authentication failures.
  - Authorisation denials.
  - Rate limit hits.
  - Malformed or unexpected message types.
  - Session hijacking indicators (IP change, concurrent sessions if disallowed).
- Never include secrets (API keys, internal tokens, database credentials) in WebSocket messages or REST responses.
- Implement session timeout and idle timeout. Force re-authentication after timeout.
- Use TLS for all connections (HTTPS for REST, WSS for WebSocket). Never allow plaintext in production.

## Client-Side (JS / HTML)

- Never store tokens or secrets in `localStorage` or `sessionStorage`. Use `httpOnly` cookies set by the server, or short-lived session tokens managed in memory only.
- Use `x-text` for all dynamic content (auto-escapes). Never use `x-html` with any server-supplied or user-supplied data.
- Apply `Content-Security-Policy` headers:
  - Restrict `script-src` to self and specific CDN origins (Alpine.js).
  - Disallow `unsafe-inline` for scripts if possible (use Alpine.js event bindings instead).
  - Set `connect-src` to the WebSocket and API origins only.
- Use Subresource Integrity (SRI) hashes for any externally-loaded JS (Alpine.js CDN).
- Never send computed results from JS to the server as authoritative values. The server must independently calculate and verify.
- Avoid exposing internal message types or payload structures in user-visible error messages.
- Sanitise any data before inserting it into DOM attributes (even via Alpine.js bindings — e.g., `x-bind:href` with user data).

## WebSocket-Specific Security

- Validate the `Origin` header on WebSocket upgrade requests to prevent cross-site WebSocket hijacking.
- Reject WebSocket connections that fail authentication within a grace period (e.g., 5 seconds after connect).
- Implement per-message-type authorisation — not all connected users may be allowed to send all message types.
- Monitor for and throttle abnormal message patterns (burst sends, unexpected types, oversized payloads).
- Close connections with appropriate close codes on security violations; do not silently ignore.
