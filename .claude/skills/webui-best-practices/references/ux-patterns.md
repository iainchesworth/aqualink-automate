# UX Patterns Reference

## Loading and Perceived Performance

- Serve static files with `Cache-Control` headers (immutable for hashed assets, short TTL for HTML).
- Inline critical CSS for above-the-fold content to avoid render-blocking.
- Show a connection-establishing state immediately — never a blank page waiting for WebSocket.
- Alpine.js is ~15 KB; load it early and let `x-cloak` hide unready elements.
- Use `x-init` on the root element to trigger WebSocket connection on page load.
- Performance targets:
  - First meaningful paint: < 1 second.
  - WebSocket connected and authenticated: < 2 seconds on 4G.
  - Time to interactive: < 2 seconds.

## Real-Time State Updates

- **Optimistic UI:** show immediate visual feedback for user actions (button state change, pending indicator); reconcile when the server confirms or rejects.
- **Animate value changes** (balance, scores, timers) rather than snapping to new values. Use CSS transitions or `x-transition`.
- **Debounce rapid server pushes** that update the same UI element (e.g., live odds ticking at 10+ updates/sec). Render at most once per animation frame.
- Show "last updated" timestamps or relative indicators for time-sensitive data.
- Handle out-of-order messages using sequence numbers (see `websocket-protocol.md`).
- For countdowns and timers: sync to server timestamp, not local clock. Drift-correct periodically.

## Error Handling UX

**User errors** (invalid bet, insufficient balance, input validation):
- Display inline, contextual, near the input or action that caused the error.
- Use `role="alert"` or `aria-live="assertive"` so screen readers announce them.
- Clear the error when the user corrects the input or takes a new action.

**System errors** (server error, unexpected state):
- Display as a banner/toast at the top of the viewport.
- Auto-dismiss when the condition resolves (e.g., reconnection succeeds).
- Include a brief, user-friendly message — never raw server errors, status codes, or stack traces.

**Connection loss:**
- Show a full-width overlay or banner: "Reconnecting..." with a spinner.
- Disable all interactive elements (buttons, inputs) while disconnected.
- Re-enable and dismiss the overlay when the connection is re-established.
- If reconnection fails after N attempts, show a "Connection lost — please refresh" message.

**Session expiry:**
- Clear all local Alpine.js store state.
- Redirect to login with a brief explanatory message.
- Do not leave stale data visible in the UI.

## Accessibility

All guidance below is both a UX best practice and a regulatory consideration for gaming products.

**Keyboard navigation:**
- All interactive elements (buttons, inputs, links, tabs, dialogs) must be reachable via Tab and operable via Enter/Space.
- Manage focus deliberately: return focus to a sensible element after dialogs close, views transition, or modals dismiss.
- Visible focus indicators on all interactive elements (never `outline: none` without a replacement).

**ARIA and live regions:**
- `aria-live="polite"` for balance updates, game state changes, non-urgent notifications.
- `aria-live="assertive"` for errors, time-critical warnings, connection loss alerts.
- `role="alert"` for responsible gaming limit notifications.
- `role="status"` for connection state indicators.
- Use `aria-label` or `aria-labelledby` for elements whose purpose isn't clear from visible text.

**Responsible gaming UI:**
- Session timers, loss limits, deposit limits, and self-exclusion links must be persistently visible.
- These elements must never be obscured by overlays, modals, or dynamic content.
- Break/timeout warnings must be announced to screen readers and visible to sighted users simultaneously.

**Colour and contrast:**
- Colour contrast ratio ≥ 4.5:1 for normal text, ≥ 3:1 for large text (18px+ bold or 24px+) and UI components.
- Never convey information by colour alone — use text labels, icons, or patterns alongside colour.

**Screen reader testing:**
- Test with VoiceOver (macOS/iOS), NVDA (Windows), and TalkBack (Android).
- Verify that all dynamic updates via WebSocket are announced appropriately.

## Responsive Design

- Design mobile-first; gaming UIs are increasingly mobile-dominant.
- Touch targets: ≥ 44×44 CSS pixels for all interactive elements.
- Test landscape and portrait orientations on mobile devices.
- Avoid hover-dependent interactions — click/tap is the primary interaction model.
- Ensure WebSocket reconnection works through mobile sleep/wake cycles.
- Use `<meta name="viewport" content="width=device-width, initial-scale=1">`.
- Test with browser DevTools throttling (CPU 4x slowdown, Slow 3G network).
