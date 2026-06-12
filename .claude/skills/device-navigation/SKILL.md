---
name: device-navigation
description: OneTouch menu navigation and screen-scraping for the Aqualink-Automate project — the SpiderEngine, MenuModel, Navigator, visit policies, and ScreenDataPage reconstruction. Use when adding or modifying navigable menu pages, page detectors/edges, the autonomous page crawler, the navigator state machine, screen-data reconstruction, or page data extraction (PageProcessors). Covers the detector-vs-edge-label rule, the pending-status counter, recovery logic, and the add-a-page workflow. Relevant to src/jandy/navigation and src/jandy/utility/screen_data_page*.
---

# Device Navigation / Screen-Scraping

How an **emulated** OneTouch controller autonomously drives a real AquaLink panel's 12-line LCD: it reconstructs each screen from wire messages, identifies the current menu page, computes a button-press path to any target, and crawls the whole menu tree to scrape config. Protocol/message mechanics are in `jandy-protocol`; the single-thread model is in `kernel-architecture`.

**When this runs:** only when emulating a OneTouch. `OneTouchDevice::ProcessControllerUpdates` short-circuits *real* devices straight to `NormalOperation` ([onetouch_device.cpp:135](src/jandy/devices/onetouch_device.cpp:135)). The **PDA device does not use this subsystem** — it uses `Capabilities::Screen` + PageProcessors for passive observation only.

## Three screen representations — don't conflate them

- **`ScreenDataPage`** (`src/jandy/utility/screen_data_page.*`) — the *live, mutable* reconstructed LCD buffer. Everything operates on this.
- **`MenuModel` / `MenuPage`** (`src/jandy/navigation/menu_model.*`) — a *static declarative graph*: every page, its detectors, and its edges. The "map".
- **`ScreenDataPageGraph`** (`src/jandy/utility/screen_data_page_graph.h`) — **legacy/dead**: a `boost::adjacency_list` used only by `test_devices_screenscraping.cpp`. Do **not** extend it for navigation work (likely cleanup target).

## 1. MenuModel (`navigation/menu_model.*`, `onetouch_menu_model.*`)

Pages are registered in `CreateOneTouchMenuModel()` via `RegisterPage({...})`. A `MenuPage` has `detectors` and `edges`:
- `Detector{ line, pattern }` — "screen line `line` *contains* substring `pattern`". Matched by `find` (padding-insensitive).
- `MenuEdge{ trigger, source, target, trigger_line, label }` — `trigger` ∈ {Select, Back, LineUp, LineDown, PageUp, PageDown, SystemTimeout, SystemService}. Self-loops (`source==target`) are cursor moves; page-transitions are Select/Back.

**THE rule — detector = page TITLE; edge `label` = menu ITEM:**
- A detector recognises *which page you're on* (usually the title line).
- An edge `label` is the menu-**item** text the Navigator moves the cursor onto, resolved at runtime by `FindLineByLabel`.
- LCD menu rows are **column-aligned**, so the same words differ in whitespace between a title and a menu row. Real example: `LightLabels` detector `{1, "Light Labels"}` (1 space) vs the `LabelAux→LightLabels` edge label `"Light   Labels"` (3 spaces). They are *intended* to disagree on spacing.
- An edge `label` may be **empty** when the row is user content (aux labels); navigation then relies on `trigger_line` alone.

**Detection tiebreaker** (`DetectPage`, `menu_model.cpp`): all detectors must match; then among matches pick **most detectors, then longest total pattern length**. `max_content_lines` rejects a page when the screen has more non-empty lines than the bound — the escape hatch so a sparse splash page (StartUp) doesn't out-rank a data-rich page sharing a weak detector. **Always detect on FIXED text**, never a user-editable line.

`FindLineByLabel` lives on the **Navigator** (operates on live content): skip empties → trim → case-insensitive **prefix** match → **word-boundary guard** (reject if the next char is alphanumeric, so `"Help"` matches `"Help   >"` but not `"HelpChoose"`). This is what makes navigation robust to **shifting menus** (optional rows that push everything down).

`FindPath(from, to)` is **BFS over page-transition edges only** (self-loops skipped). A page with **no incoming Select edge is unreachable** — `FindPath` returns empty and the crawler silently skips it (this is the intended way to make a page un-crawlable, e.g. `CustomLabel`).

## 2. Navigator (`navigation/navigator.*`)

State machine: `Idle, Syncing, Navigating, WaitingForPage, MovingCursor, EnteringPassword, Reorienting, AtDestination, Failed`. Tick is `OnPageUpdate(content, highlighted_line) -> optional<NavKeyCommand>`.

- **Sync first**: requires **3 consecutive identical detections** (`StartSync`/`HandleSyncing`), skipping transient pages (StartUp/Service/TimeOut). You can't navigate from a splash screen.
- **Pending-status counter `m_PendingStatusMessages`** paces commands against the wire. `OnStatusMessageReceived()` decrements it (once per `JandyMessage_Status`); commands won't advance until 0. **Set to 2 for page transitions (Select/Back)** (a full redraw spans two Status messages), **1 for cursor/scroll moves**. Getting this wrong → premature page-mismatch (spurious recovery) or a hang.
- **`HandleWaitingForPage` must re-check `m_State` after calling `HandleUnexpectedPage`** — the latter may have already recomputed a valid path; ignoring the re-check drops it.
- **Recovery**: `HandleUnexpectedPage` bumps `m_RecomputeCount` (cap `MAX_RECOMPUTE_COUNT`=50 → Failed) and re-paths from the actual page; only if that fails does it `InitiateRecovery` (bumps `m_RecoveryAttempts`, cap 3; `Reorienting` walks Back toward `System`, cap `MAX_BACK_PRESSES`=10). OneTouch-style pages have **no Back edge** — recovery uses a `Select`-to-`System` edge instead.
- **`NavigateTo()` MUST reset** `m_RecomputeCount`, `m_RecoveryAttempts`, `m_RecoveryBackPresses` (plus `m_PendingStatusMessages`, `m_Path`, `m_PathIndex`, `m_SelectTarget`, `m_NavigatingToItem`) — or a second navigation inherits an exhausted recovery budget and fails spuriously.
- **Cursor guards**: `MAX_CURSOR_MOVES`=15 (wrap) / `MAX_CURSOR_STUCK_COUNT`=5 — in normal mode accept the current line and proceed; in `NavigateToItem` mode **fail** (the item genuinely isn't there). Don't raise these blindly.
- `NavigateToItem(page, line, label, select_target)` positions the cursor (by label, else line), scrolls up to `MAX_ITEM_SCROLL_ATTEMPTS`=6 if `SupportsKey(PageDown)`, optionally presses Select for a sub-page (used for multi-instance pages). Password entry is a placeholder (no digit-wheel parsing yet).

## 3. SpiderEngine (`navigation/spider_engine.*`, `visit_policies.*`)

Autonomous crawler owning `MenuModel&`, `Navigator&`, `unique_ptr<VisitPolicy>`. States: `Idle, Syncing, NavigatingToNext, CapturingPage, Complete, Failed`. `ProcessStep(content, line)` is ticked by the device each frame: sync → pick nearest unvisited target (shortest `FindPath`) → drive Navigator → on success `CapturingPage` (policy `OnPageReached`), on failure bump `m_NavigationFailures` (cap `MAX_NAVIGATION_FAILURES`=3) and mark visited to skip.

- **Multi-instance pages** (`MenuPage::multi_instance`, only `LabelAux`): same `PageId`, different content per incoming Select edge. Tracked per-edge; marked fully visited only when **every** incoming edge is crawled. Forgetting `multi_instance=true` scrapes just the first aux.
- **`SpiderEngine::OnStatusReceived()` is intentionally empty** — the device already calls `Navigator::OnStatusMessageReceived()`. Decrementing again double-counts and breaks pacing.
- Reachability is graph-derived, not policy-derived: `FullDiscoveryVisitPolicy` allows almost everything; exclude a page by omitting its incoming edge, not by hacking the policy.

## 4. Screen data (`utility/screen_data_page*`, `devices/capabilities/screen.*`)

- `ScreenDataPage` = `vector<RowType{ Text, HighlightState, HighlightRange }>`. `Highlight(line)` **clears all other highlights** (single-cursor invariant); `0xFF` clears all. `ShiftLines(...)` models vertical scroll.
- `ScreenDataPage_Processor{ page_type, MenuMatcherDetails{line, substr}, callback }` — data-extraction handlers; `CanProcess` is a case-insensitive substring search. **Separate from MenuModel detectors** (a second, parallel matching system): the model drives *navigation*, processors drive *data extraction*. They usually mirror each other and you maintain both.
- `ScreenDataPageUpdater` is a Boost.Statechart turning wire events (Clear/Highlight/HighlightChars/Shift/Update) into page mutations.
- `Screen` capability owns the page/updater/processors and the **`ScreenModes`** lifecycle: events → `Updating`; a Status message → `UpdateComplete`; `ProcessScreenUpdates()` then resets page type, runs matching processors (setting `DisplayedPageType`), and returns to `Normal`. `DescribeScreen()` serialises it to JSON for diagnostics.

Note: **Status messages are the only carrier for key commands.** The device sends a queued `KeyCommand` only on a Status message and with `AckTypes::V1_Normal` (the controller ignores key presses in V2/0x80 ACKs).

## 5. Checklist — add or modify a navigable page

1. Add a `PageId` in `menu_model.h` (if new) and a matching `ScreenDataPageTypes` in `screen_data_page_processor.h` (kept 1:1; uniqueness is test-enforced).
2. `RegisterPage({...})` in `onetouch_menu_model.cpp`: detectors on **fixed title text** (add a 2nd detector or `max_content_lines` to break collisions); one Select/Back edge per transition with the **menu-item** `label` exactly as rendered (or `""` for user rows); **always add LineUp+LineDown self-loops** (test-enforced); add PageUp/PageDown if the page shows `^^ More vv`.
3. Make it reachable: add the incoming Select edge on the parent page (no incoming edge = unreachable).
4. Multi-instance? Set `multi_instance=true` + one incoming edge per instance.
5. Register the matching `ScreenDataPage_Processor` in the `OneTouchDevice` ctor and implement `PageProcessor_<Name>` to extract into the DataHub.
6. Make un-crawlable-but-detectable pages by omitting the incoming edge (not via the policy).
7. Update `assets/web/api/swagger.yaml` if you changed the diagnostics JSON shape (per CLAUDE.md).
8. **Test** (target `testaqualink-automate`): structural assertions + `FindPath(System, NewPage)` non-empty in `test_navigation_menu_model.cpp`; a `DetectPage` test; a realistic-content entry in `test_spider_engine.cpp`'s registry (and bump the crawl visit-count if reachable). Build per `cmake-build-system`.

## 6. Gotchas

- Detector = TITLE (substring); edge label = menu ITEM (trim + case-insensitive prefix + word-boundary). Whitespace legitimately differs.
- Empty edge label is intentional for user-content rows; nav falls back to `trigger_line`.
- Tiebreaker: most detectors, then longest total pattern length. `max_content_lines` saves sparse pages.
- Pending-status counter: Select/Back=2, cursor/scroll=1. Decremented once per Status message.
- Don't double-count Status (`SpiderEngine::OnStatusReceived` is deliberately empty).
- Key commands ride only on Status messages, with `V1_Normal` ACK; don't clear `m_KeyCommand_ToSend` on non-Status messages.
- `NavigateTo()` must reset the recompute/recovery counters.
- `HandleWaitingForPage` must re-check state after `HandleUnexpectedPage`.
- Sync needs 3 consecutive detections; skips transient pages.
- Reachability is graph-derived — exclude a page by omitting its incoming edge.
- Cursor wrap/stuck guards accept-and-proceed (normal) but fail (item mode).
- OneTouch-style pages have no Back; recovery uses Select→System.
- `multi_instance` pages need all incoming edges crawled before "visited".
- Two parallel matching systems (MenuModel detectors vs PageProcessor matchers) — keep both in sync or you get navigate-but-can't-scrape.
- Loop ceilings (`MAX_RECOMPUTE_COUNT`50, `MAX_RECOVERY_ATTEMPTS`3, `MAX_BACK_PRESSES`10, `MAX_WAIT_CYCLES`100, `MAX_ITEM_SCROLL_ATTEMPTS`6) end in `Failed` by design.
- `ScreenDataPage::Highlight` clears other highlights (single cursor).
- Engine runs only for emulated OneTouch; PDA doesn't use it; `ScreenDataPageGraph` is dead.

## Key files

- Model: `src/jandy/navigation/{menu_model,onetouch_menu_model}.{h,cpp}`
- State machine: `src/jandy/navigation/navigator.{h,cpp}` (`FindLineByLabel`, `NavigateTo` resets, `HandleWaitingForPage`/`HandleUnexpectedPage`)
- Crawler: `src/jandy/navigation/{spider_engine,visit_policies}.{h,cpp}`
- Screen data: `src/jandy/utility/{screen_data_page,screen_data_page_processor,screen_data_page_updater}.*`, `src/jandy/devices/capabilities/screen.{h,cpp}`
- Device wiring: `src/jandy/devices/onetouch_device.{h,cpp}`, `src/jandy/devices/onetouch/onetouch_messageprocessors.cpp`
- Tests: `test/unit/navigation/*`, `test/unit/devices/test_devices_screenscraping.cpp`
