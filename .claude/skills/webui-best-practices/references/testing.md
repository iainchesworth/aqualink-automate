# Testing Reference

## Testing Pyramid

```
Layer 4: E2E (Playwright)         — full browser → WebSocket → C++ round-trips
Layer 3: JS unit (optional)       — Alpine.js store logic in isolation
Layer 2: C++ integration          — start server, exercise REST + WebSocket with test client
Layer 1: C++ unit (Catch2/GTest)  — pure business logic, no network
```

## Layer 1 — C++ Unit Tests

- Test all business logic functions in isolation (game rules, validation, state machines, calculations).
- No network, no WebSocket, no HTTP — pure function in, value out.
- Use `std::expected`-based returns; test both success and error paths.
- Write a test for every new C++ function or state machine transition.
- Run with: `ctest --test-dir build/ --output-on-failure`

## Layer 2 — C++ Integration Tests

- Start the server in test mode (separate port, test database/fixtures).
- Exercise REST endpoints with a test HTTP client: validate status codes, response envelope shape, and payload correctness.
- Exercise WebSocket connections with a test WS client:
  - Connect, authenticate, send messages, validate responses.
  - Test message ordering and sequence handling.
  - Test disconnection cleanup (connect, subscribe, disconnect, verify server state cleaned up).
  - Test rate limiting (send burst, verify throttle response).
- Validate JSON schemas of all responses against expected shapes.
- Write a test for every new API endpoint or WebSocket message handler.

## Layer 3 — JS Unit Tests (Optional)

- If Alpine.js store logic is complex (derived state calculations, message routing, reconnection logic), test it in Node.js or a browser test runner.
- Mock the WebSocket connection; verify that stores dispatch the correct messages and update state correctly in response to incoming messages.
- Useful when store logic has branching, error handling, or sequence management.

## Layer 4 — E2E Tests (Playwright)

- Full stack: real browser, real WebSocket connection, real C++ backend.
- Test critical user flows end-to-end:
  - Page load → WebSocket connect → initial state rendered.
  - User action → message sent → server processes → response rendered.
  - Connection loss simulation → reconnection → state recovery.
  - Error scenarios: invalid input, session expiry, rate limiting.
- Test accessibility: keyboard navigation, focus management, screen reader announcements.
- Run before marking any cross-stack feature complete.
- Run with: `npx playwright test`

## What Claude Code Should Do

- After writing C++ business logic: write Layer 1 tests.
- After writing/modifying an API endpoint or WS handler: write Layer 2 tests.
- After writing a new user-facing feature: write Layer 4 tests.
- Run Layers 1 + 2 before marking a backend change complete.
- Run Layer 4 before marking a cross-stack feature complete.
- If a test fails, fix the code (not the test) unless the test is genuinely wrong.
