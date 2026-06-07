---
name: backend-observability
description: Backend observability for the Aqualink-Automate project — Tracy profiling (ProfilingUnitFactory zones/frames/domains, the no-op fallback, frame marks) and channel-based logging (the Channel enum, runtime severity filter, global loggers, lazy log evaluation, std::formatter integration). Use when instrumenting a handler or hot path, adding or changing log statements, adding a log Channel, or reasoning about ENABLE_PROFILING / log filtering. Relevant to src/core/profiling, src/core/logging, and src/core/interfaces/iprofiler*.
---

# Backend Observability — Profiling & Logging

Two facades over heavy third-party machinery (Tracy / Boost.Log), both compile-time-gated so production can strip them. This is about the *conventions*: instrument hot paths with a zone, log on the right channel. The web-layer observability (Sentry, metrics endpoints) is a separate concern in `webui-best-practices`.

## A. Profiling (Tracy)

**Two layers named alike — don't confuse them:**
- `Interfaces::IProfiler` (`src/core/interfaces/iprofiler.*`, impls `tracy_profiler.*` / `noop_profiler.*`, via `Factory::ProfilerFactory`) — the *process* API: `StartProfiling`, `Message`, `PlotValue`, `EmitFrameMark`, `AppInfo`.
- **`Factory::ProfilingUnitFactory`** (`src/core/profiling/factories/profiling_unit_factory.*`) — the **call-site** API every handler/hot path uses. Returns `Types::ProfilingUnitTypePtr` (a `unique_ptr<IProfilingUnit>`).

Both are singletons (`::Instance()`), both in namespace `AqualinkAutomate::Factory`.

**The zone pattern** — open a zone as the first statement of a function/handler; it stays open for the lifetime of the local (RAII closes it). Canonical model: `src/core/http/webroute_equipment_devices.cpp`:
```cpp
auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone(
    "WebRoute_EquipmentDevices::OnRequest", std::source_location::current());
// ... work ...
zone->Value(resp.body().size());   // attach a scalar metric (size/count — never payloads)
```
- `CreateZone(name, src_loc, colour = UnitColours::NotSpecified)` — **`src_loc` has no default** (unlike `CreateDomain`/`CreateFrame`); always pass `std::source_location::current()`. Forgetting it is a compile error.
- Name as `"Type::Method"`; for sub-phases use a nested `zone` in an inner `{ }` scope named `"Type::Method -> phase"` (model: `Screen::ProcessScreenUpdates` in `screen.cpp`).
- `zone->Value(uint64_t)` / `zone->Text(string_view)` annotate the open zone. **No-ops outside Tracy** (base `IProfilingUnit` does nothing; only `TracyZone` forwards) — safe but don't assume the metric was recorded in a non-Tracy build.
- Per-iteration loop boundaries use the *process* API instead: `profiler.Get()->EmitFrameMark("MainLoop")` (once per main-loop iteration in `aqualink-automate.cpp`).

**Gating / no-op:** `ENABLE_PROFILING` (build option) defines `TRACY_ENABLE` and links `Tracy::TracyClient` (see `cmake-build-system`). When off, the factories return no-op generators and every `CreateZone` compiles to a do-nothing — call sites are unchanged. **`ENABLE_PROFILING` and `ENABLE_SANITIZERS` are mutually exclusive** (configure-time `FATAL_ERROR`); to run sanitizers, reconfigure with profiling off.

## B. Logging

- **Channels** (`logging/logging_channels.h`) — `enum class Channel`: `Certificates, Coroutines, Developer, Devices, Equipment, Exceptions, Main, Messages, Mqtt, Navigation, Options, Platform, Profiling, Protocol, Scraping, Serial, Signals, Web`. Pick by subsystem: `Web` (HTTP/WS), `Devices`/`Equipment`, `Messages`/`Protocol` (RS-485), `Navigation`/`Scraping`, `Signals`, `Mqtt`, `Serial`, `Options`, `Main` (lifecycle).
- **Severity** (`logging_severity_levels.h`) — **7 levels**: `Trace, Debug, Info, Notify, Warning, Error, Fatal` (note `Notify` between Info and Warning).
- **Calls** — free function templates (not macros) at global scope: `LogTrace/LogDebug/LogInfo/LogNotify/LogWarning/LogError/LogFatal(Channel, message)`. `source_location` defaults to the call site — don't pass it. `using namespace AqualinkAutomate::Logging;` is the file convention.
- **Lazy evaluation** — `Log()` checks `SeverityFiltering::ShouldLog(channel, severity)` **first**, then, if the message is `std::invocable`, invokes it. So pass a **lambda** for expensive messages so they're only built when the line will actually emit:
  ```cpp
  LogTrace(Channel::Protocol, [&]{ return std::format("decoded: {}", ExpensiveDump(buf)); });
  ```
  (Eagerly `std::format`-ing into a hot-path `LogTrace` pays the format cost regardless of the filter — use the lambda there.)
- **Runtime filter** — `SeverityFiltering` (`logging_severity_filter.*`): `SetGlobalFilterLevel`, `SetChannelFilterLevel(Channel, Severity)`. `main` sets `Info` at startup; the `WebRoute_Diagnostics_Logging` route adjusts it live.
- **Custom types** — provide a `template<> struct std::formatter<T>` (pattern: `src/jandy/formatters/*.h`), then `LogX(channel, std::format("...{}", obj))`. That's the bridge: `std::formatter` → `std::format` → `LogX`.
- **Profiling bridge** — `Warning`+ messages also emit a coloured Tracy `Message`, so over-using `Warning` pollutes the profiler timeline.
- **Never log secrets or full payloads** — log identifiers, sizes, enum names, decoded *summaries*. Not raw wire buffers, credentials, certs, or full request/response bodies.

## Checklists

**Instrument a handler / hot path**
1. `#include "profiling/factories/profiling_unit_factory.h"` (+ `<source_location>`).
2. First statement: `auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("Type::Method", std::source_location::current());`
3. Sub-phases → nested `zone` in an inner scope, named `"Type::Method -> phase"`.
4. Optionally `zone->Value(size_or_count)` near the end (never payloads). Optional `UnitColours` 3rd arg.
5. Keep the variable named `zone`; let RAII close it (a discarded temporary measures nothing). No build edits needed.

**Add a log statement**
1. `#include "logging/logging.h"` + `using namespace AqualinkAutomate::Logging;`.
2. `LogInfo(Channel::X, std::format(...))` — channel by subsystem, severity by intent (Trace/Debug carry file:line; Warning+ also mark the Tracy timeline).
3. Expensive message on a hot path → pass a **lambda**, not a pre-formatted string.
4. Custom types → ensure a `std::formatter<T>` exists. Never log secrets/payloads.

**Add a new Channel — touch all four sites or it silently misbehaves**
1. `logging_channels.h` — add the enumerator.
2. `global_logger.h` — add `BOOST_LOG_GLOBAL_LOGGER(GlobalLogger_<Name>, Logger);`.
3. `logging.h` — add a `case Channel::<Name>: return GlobalLogger_<Name>::get();` (else it falls through to `Main`).
4. `logging_severity_filter.cpp` — add `{Channel::<Name>, DEFAULT_SEVERITY},` to the map (a missing key default-inserts as `Trace` — the latent bug currently affecting `Coroutines`/`Developer`).

## Gotchas

- **Profiling vs sanitizers**: mutually exclusive (configure-time FATAL). Reconfigure `-DENABLE_PROFILING=OFF` to use ASan/UBSan.
- **The process API (`ProfilerFactory::Get()`) returns NoOp unless `SetDesired()` was called** — which `main` doesn't do today, so `PlotValue`/`Message`/frame marks are effectively inert at runtime. Call-site zones still compile and are safe regardless.
- `zone->Value()`/`Text()` are no-ops outside a Tracy build.
- `CreateZone` has no `src_loc` default — always pass `std::source_location::current()`.
- Zone lifetime = the variable's scope; bind it (don't discard the temporary); use inner `{ }` for sub-spans.
- Severity has **7** levels (incl. `Notify`); `Warning`+ also hit the Tracy timeline — don't over-use it.
- The channel-filter map must track the `Channel` enum (missing key ⇒ defaults to `Trace`).
- **Unused-`logging.h` smell**: including it pulls in heavy Boost.Log (+ profiler headers) without any `LogX` call — drop the include if no log call remains (this was part of the O1 diagnostics-handler finding); add it when you introduce the first log call.

## Key files

`src/core/interfaces/{iprofiler,iprofilingunit}.{h,cpp}`, `src/core/profiling/factories/{profiling_unit_factory,profiler_factory}.{h,cpp}`, `src/core/profiling/{profiling.{h,cpp},tracy_profiler.*,noop_profiler.*,profiling_units/*,types/profiling_types.h}`, models `src/core/http/webroute_equipment_devices.cpp` + `src/jandy/devices/capabilities/screen.cpp` + `src/aqualink-automate.cpp` (registration/frame marks); `src/core/logging/{logging.h,logging_channels.h,logging_severity_levels.h,logging_severity_filter.*,global_logger.h,logging_initialise.cpp,logging_formatter.cpp}`, `std::formatter` examples `src/jandy/formatters/*.h`.
