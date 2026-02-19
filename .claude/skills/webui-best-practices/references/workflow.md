# Claude Code Workflow Reference

## Plan Before Coding

For any non-trivial change — especially cross-boundary work (C++ + JS) — plan first.

**Use Plan Mode** (`Shift+Tab` twice) to have Claude research and plan without writing code.

**Or write a plan to a file** for complex features:

```
Prompt: "Plan adding a live bet-slip feature. The C++ backend must
track pending selections, validate combinations, and push updates
via WebSocket. The Alpine.js store manages UI state. Write the plan
to plan.md covering: C++ message types, WebSocket handler, Alpine
store, HTML template, and test plan. Do not write code yet."
```

Break the plan into phases and confirm each before proceeding:
1. C++ data model and business logic.
2. WebSocket message types and handlers.
3. Alpine.js store and JS connection logic.
4. HTML template and CSS.
5. Tests at all layers.

## Verification After Every Change

1. **C++ modified** → build and run `ctest`.
2. **API/WS handler modified** → rebuild, run integration tests.
3. **JS/HTML/CSS modified** → run `eslint`, check in browser or run Playwright.
4. **Message schema changed** → update BOTH C++ and JS, run E2E.
5. **Always** → run `clang-format`, `clang-tidy`, `eslint` before marking complete.

Do not skip verification. The C++/JS boundary has multiple failure points.

## Hooks for Automated Quality Gates

Configure hooks so formatting and linting run automatically after edits:

```json
{
  "post_edit": [
    {
      "pattern": "src/**/*.cpp",
      "command": "clang-format -i $FILE && clang-tidy $FILE"
    },
    {
      "pattern": "src/**/*.hpp",
      "command": "clang-format -i $FILE"
    },
    {
      "pattern": "web/js/**/*.js",
      "command": "eslint --fix $FILE"
    }
  ]
}
```

## Subagents for Focused Review

Create specialised subagents for cross-cutting concerns:

**Protocol reviewer** (`.claude/agents/protocol-reviewer.md`):
```
---
name: protocol-reviewer
description: Reviews WebSocket protocol changes for consistency and safety
model: sonnet
---
You are a real-time systems specialist. Review changes for:
- Message type consistency between C++ enum and JS constants
- Missing server-side validation of incoming messages
- Missing reconnection/error handling on the JS side
- Race conditions in concurrent WebSocket message handling
- Proper cleanup of server-side state on disconnection
```

**Accessibility reviewer** (`.claude/agents/a11y-reviewer.md`):
```
---
name: a11y-reviewer
description: Reviews HTML templates for accessibility compliance
model: sonnet
---
You are an accessibility specialist for a regulated gaming product. Review for:
- Keyboard navigability of all interactive elements
- ARIA live regions for WebSocket-driven dynamic content
- Focus management after state transitions and dialog dismissal
- Colour contrast compliance (4.5:1 text, 3:1 UI components)
- Responsible gaming elements persistently visible and announced
```

## Context Management

C++ codebases are token-hungry. Manage context deliberately:

- Use **`/add-dir`** to scope sessions to relevant directories:
  - Backend session: `src/ws/` + `src/core/`
  - Frontend session: `web/js/` + `web/templates/`
  - Cross-boundary: `src/ws/` + `web/js/stores/`
- Use **`/compact`** when context fills up mid-session.
- Break features into focused sessions: C++ handler first, then JS store, then HTML template.
- Use **`/rewind`** immediately when Claude goes down a wrong path — don't let it continue.
- For large investigations, use subagents to explore specific areas without consuming main session context.

## Custom Slash Commands

**New WebSocket message type** (`.claude/commands/new-message.md`):
```
Create a new WebSocket message type called $ARGUMENTS:
1. Add the enum value to src/ws/messages.hpp with serialisation/deserialisation.
2. Add the constant to web/js/message-types.js.
3. Create a stub handler in src/ws/handlers/.
4. Add a corresponding dispatch case in the Alpine.js connection store.
5. Add the message documentation to PROTOCOL.md (type, direction, payload shape).
6. Write a C++ integration test for the handler.
7. Build and run tests to verify.
```

**New REST endpoint** (`.claude/commands/new-endpoint.md`):
```
Create a new REST endpoint: $ARGUMENTS
1. Add the route handler in src/api/.
2. Implement input validation and return a consistent JSON envelope.
3. Add any required data model functions in src/models/.
4. Write C++ unit tests for the business logic.
5. Write a C++ integration test exercising the endpoint.
6. Update any consuming JS code in web/js/ if applicable.
7. Build and run tests to verify.
```

**New Alpine.js store** (`.claude/commands/new-store.md`):
```
Create a new Alpine.js store called $ARGUMENTS:
1. Create web/js/stores/$ARGUMENTS.js with Alpine.store('$ARGUMENTS', { ... }).
2. Register it in web/js/init.js.
3. Add the script tag in the correct load order in the HTML.
4. If the store consumes WebSocket messages, add dispatch cases in the connection store.
5. Run eslint to verify.
```

## Session Patterns

**"Explore first" for unfamiliar code:**
```
Prompt: "Explore the WebSocket handler code in src/ws/. Summarise:
what message types exist, how sessions are managed, and where
the handler dispatch logic lives. Don't change anything."
```

**"Interview me" for requirements:**
```
Prompt: "Interview me about the requirements for the new tournament
feature. Ask about game rules, state transitions, edge cases, and
compliance requirements. Keep asking until we've covered everything,
then write a spec to SPEC.md."
```

**"Checkpoint" before risky changes:**
Before a large refactor, verify your starting point is clean:
```
Prompt: "Build the project and run all tests. Report the results.
Do not make any changes."
```
