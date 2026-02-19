# Alpine.js Frontend Reference

## Store Architecture

- One store per bounded concern:
  - `Alpine.store('connection')` — WebSocket lifecycle, connection state, send/receive.
  - `Alpine.store('game')` — current game state, round info, display values.
  - `Alpine.store('user')` — session info, balance (display-only), preferences.
  - `Alpine.store('notifications')` — toasts, errors, system messages.
- Stores hold reactive state and dispatch intents via the connection store.
- Stores must **never** perform authoritative game logic — only display transformations (formatting currency, calculating display-only derived values).
- Keep stores in individual files under `web/js/stores/`; register them in `web/js/init.js`.
- Use `Alpine.store()` to share state across components; avoid duplicating state in `x-data`.
- Store methods that send WebSocket messages should call `Alpine.store('connection').send(type, payload)` — centralise all outbound traffic.

## HTML Templates

- Use semantic HTML: `<section>`, `<article>`, `<nav>`, `<dialog>`, `<main>`, `<header>`, `<footer>`.
- Alpine.js directives are the templating layer:
  - `x-data` for component-local state.
  - `x-bind` / `:attr` for dynamic attributes.
  - `x-on` / `@event` for event handlers.
  - `x-show` for toggling visibility of existing DOM elements.
  - `x-if` (inside `<template>`) for conditional rendering that adds/removes elements.
  - `x-for` (inside `<template>`) for list rendering.
  - `x-text` for dynamic text content (**auto-escapes; always prefer over x-html**).
  - `x-html` — **never use with server-supplied data** (XSS risk). Only for trusted static HTML fragments.
  - `x-transition` for animated state changes.
  - `x-ref` for imperative DOM access when needed.
- Use `x-cloak` on elements that should be hidden until Alpine initialises:
  ```css
  [x-cloak] { display: none !important; }
  ```
- Never embed business logic in templates — `x-on` handlers should call store methods.
- Avoid deeply nested `x-data` scopes; if a template section is complex enough to need its own state, extract it into a reusable Alpine component or a store.

## CSS Rules

- Use CSS custom properties for theming:
  ```css
  :root {
    --color-primary: #...;
    --color-error: #...;
    --spacing-unit: 8px;
    --radius-default: 4px;
  }
  ```
- Mobile-first responsive design. Breakpoints: 320px, 768px, 1024px.
- Use CSS Grid and Flexbox for layout. No floats for structural layout.
- Animate with `transform` and `opacity` for GPU acceleration. Avoid animating `width`, `height`, `top`, `left`.
- Keep specificity low — use BEM naming or a utility class system consistently.
- Dark mode via `prefers-color-scheme` media query plus a manual toggle stored in `Alpine.store('user')`.
- Colour contrast: ≥ 4.5:1 for normal text, ≥ 3:1 for large text and interactive components.

## JS File Organisation (No Build Step)

Since there's no JS bundler, manage complexity through convention:

```
web/js/
├── message-types.js    — WebSocket message type constants (mirrors C++ enum)
├── ws-client.js        — WebSocket connection manager (reconnection, queueing)
├── stores/
│   ├── connection.js   — Alpine.store('connection')
│   ├── game.js         — Alpine.store('game')
│   ├── user.js         — Alpine.store('user')
│   └── notifications.js — Alpine.store('notifications')
├── utils/
│   ├── format.js       — Display formatting (currency, dates, durations)
│   └── dom.js          — DOM helper utilities
└── init.js             — Alpine.start(), store registration, WebSocket init
```

**Load order in HTML** (matters without a bundler):
1. Alpine.js (CDN or vendored)
2. `message-types.js`
3. `ws-client.js`
4. Store files (any order — they register on `Alpine.store`)
5. `init.js` (calls `Alpine.start()` last)

Alternatively, use `type="module"` with ES modules and an import map for cleaner dependency management.

## Alpine.js Component Patterns

**Connection-aware component:**
```html
<div x-data x-show="$store.connection.status === 'connected'">
  <!-- Only visible when connected -->
</div>
<div x-data x-show="$store.connection.status === 'reconnecting'"
     class="connection-banner" role="alert">
  Reconnecting...
</div>
```

**Reactive display from store:**
```html
<span x-data x-text="$store.game.formattedBalance"
      aria-live="polite"></span>
```

**Intent dispatch:**
```html
<button x-data @click="$store.game.placeBet()"
        :disabled="!$store.connection.isConnected || $store.game.betLocked">
  Place Bet
</button>
```
