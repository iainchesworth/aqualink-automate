# C++23 Backend Reference

## C++23 Feature Policy

Adjust this list to match your compiler version (GCC 14+ / Clang 18+) and library support.

**Approved:**
- `std::expected` — all fallible operations; prefer over exceptions in API/WS handlers
- `std::format` / `std::print` — logging and diagnostic output
- `std::flat_map` / `std::flat_set` — small-to-medium lookup tables in hot paths (cache-friendly)
- Deducing this — CRTP elimination in handler base classes
- Multidimensional subscript operator — game grid/board logic where applicable
- `std::generator` — streaming result sets to WebSocket clients
- `if consteval` / expanded `constexpr` — compile-time validation of message schemas
- `std::mdspan` — multidimensional views over game/display buffers

**Use with care:**
- `std::stacktrace` — debug builds only; strip from production
- `std::jthread` in request handlers — prefer the framework's async model (io_context, event loop)
- Exceptions — avoid in hot paths and handler return paths; use `std::expected` instead

**Not yet supported / avoid:**
- Features not implemented in your Emscripten or compiler version — verify before using

## API Handler Patterns

Every REST endpoint must:
1. Parse and validate input before any processing.
2. Return a consistent JSON envelope: `{ "ok": true/false, "data": {}, "error": "" }`.
3. Map `std::expected` error types to appropriate HTTP status codes (400 for validation, 401/403 for auth, 500 for internal).
4. Log the request with a correlation ID.
5. Never leak internal state (pointers, raw session IDs, stack traces) in responses.

## WebSocket Handler Patterns

Every WebSocket message handler must:
1. Validate the message schema before processing.
2. Authenticate and authorise the session before acting.
3. Return results via the same WebSocket connection (or broadcast to subscribed sessions).
4. Use `std::expected` internally; convert errors to typed error messages sent to the client.
5. Implement per-session rate limiting.
6. Handle disconnection cleanup (cancel timers, remove subscriptions, release resources).

## JSON Serialisation

- Use a single JSON library consistently throughout the codebase.
- Define serialisation/deserialisation functions adjacent to the data model (`src/models/`), not in handler code.
- Validate incoming JSON schema before deserialising into C++ types.
- Never serialise internal server state to the client.
- Use `std::expected` to propagate parse/validation errors — don't throw.
- For high-throughput paths, consider `simdjson` for parsing and a fast serialiser (e.g., `glaze`) for output.

## Error Handling Strategy

- Handler layer: `std::expected<ResponseT, ErrorT>` — never throw.
- Core business logic: may use exceptions internally if caught at the handler boundary.
- Map error types to:
  - REST: HTTP status codes + JSON error envelope.
  - WebSocket: typed error message (`{ "type": "error", "payload": { "code": "...", "message": "..." } }`).
- Log all errors with severity, correlation ID, and session ID.
