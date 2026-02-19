# Observability Reference

## Backend Logging (C++)

- Use structured JSON logging with consistent fields:
  - `timestamp` (ISO 8601)
  - `severity` (debug, info, warn, error, fatal)
  - `correlation_id` (per-request or per-session)
  - `session_id`
  - `component` (api, ws, core, auth)
  - `message`
  - `data` (structured context — never raw user input)
- Log WebSocket lifecycle events: connect, authenticate, disconnect (with close code and duration).
- Log message types received and sent (type field only, not full payloads) at debug level.
- Log all security events at warn or error level (auth failures, rate limits, malformed messages).
- Never log secrets, tokens, passwords, or full user payloads at any level.
- Expose endpoints:
  - `GET /health` — server up, dependencies reachable (database, external services).
  - `GET /metrics` — connection count, message throughput, handler latency percentiles, error rates.

## Backend Debugging (C++)

- Debug build flags: `-g -fsanitize=address,undefined -DDEBUG`
- Enable AddressSanitizer (ASan) and UndefinedBehaviorSanitizer (UBSan) in CI builds.
- Enable ThreadSanitizer (TSan) in CI if using threads.
- Use `std::stacktrace` in debug builds for richer error context.
- Profile WebSocket handler latency with high-resolution timers (`std::chrono::steady_clock`).
- Monitor memory usage and connection count; alert if either exceeds budget.

## Frontend Debugging (JS)

- In dev mode, log WebSocket message types (not full payloads) to the browser console.
- Use the Alpine.js DevTools browser extension to inspect store state and component data.
- Add a debug panel behind a feature flag showing:
  - Connection state and duration.
  - Last N message types sent/received (not payloads).
  - Round-trip latency (timestamp in server message vs. local receipt time).
  - Sequence number tracking (last processed seq per channel).
- Use the Performance API to measure time from user action to UI update:
  ```javascript
  performance.mark('bet-click');
  // ... after server confirms and UI updates:
  performance.mark('bet-confirmed');
  performance.measure('bet-round-trip', 'bet-click', 'bet-confirmed');
  ```
- Monitor for Alpine.js reactivity issues: unexpected re-renders, stale store state, missing `x-cloak`.

## Production Monitoring

- Use Sentry (or equivalent) with source maps for JS error tracking.
- Track C++ backend crashes and panics with core dump collection or crash reporting.
- Monitor WebSocket connection churn (connects/disconnects per minute).
- Alert on: error rate spikes, connection count drops, handler latency P99 exceeding threshold.
- Dashboard: active connections, messages/sec, error rate, handler latency P50/P95/P99.
