---
name: kernel-architecture
description: Kernel architecture for the Aqualink-Automate project — the HubLocator dependency-injection container, the DataHub/EquipmentHub/StatisticsHub/PreferencesHub state hubs, hub events, boost::signals2 status propagation, and the single-threaded cooperative execution model. Use when adding a hub, resolving hubs in a constructor, publishing a status/event out to WebSocket/MQTT consumers, or reasoning about thread-safety and the main poll() loop. Relevant to src/core/kernel, src/core/interfaces (ihub/istatus/istatuspublisher), and src/aqualink-automate.cpp.
---

# Kernel Architecture

The "kernel" is the app's dependency-injection container plus the shared state hubs and the event channels that fan state changes out to transports (WebSocket, MQTT). Protocol mechanics are in `jandy-protocol`; build/run in `cmake-build-system`.

## 1. HubLocator — the DI container (`src/core/kernel/hub_locator.h`)

A tiny service-locator (header-only; `.cpp` is empty) storing type-erased `shared_ptr`s and resolving by **exact static type**.

- **`Register<T>(shared_ptr<T>) -> HubLocator&`** — wraps in `Hub::WrapperImpl<T>`; chainable.
- **`Find<T>() -> shared_ptr<T>`** — linear scan + `dynamic_pointer_cast<WrapperImpl<T>>`; **throws `Exceptions::Hub_NotFound`** when absent.
- **`TryFind<T>() -> shared_ptr<T>`** — same, but **returns `nullptr`** when absent.
- **`Unregister<T>() -> HubLocator&`** — `erase_if` matching wrappers; chainable.

Consequences to internalise:
- **Resolution is by exact static type.** A hub registered as `shared_ptr<DataHub>` is found by `Find<DataHub>()`, **not** `Find<IHub>()`. To expose something under an interface, register it explicitly: `hub_locator.Register<Interfaces::ICommandDispatcher>(dispatcher)` (the one place this is done).
- **One instance per type; no overwrite.** Re-registering the same `T` inserts a second wrapper and `Find` returns an arbitrary one — treat double-registration as a bug.

**Where registered** — `src/aqualink-automate.cpp` `main` ("INFORMATION DISTRIBUTION HUBS"): create the four hubs, `hub_locator.Register(data_hub).Register(equipment_hub).Register(preferences_hub).Register(statistics_hub);`, then construct `CommandDispatcher` (needs data+equipment hubs) and `Register<ICommandDispatcher>(...)`. The locator is a `main`-scoped local passed **by reference** into subsystem `Configure(...)` functions — there is no global/singleton.

**Where consumed** — constructors take `Kernel::HubLocator&` and cache handles: `m_DataHub = hub_locator.Find<Kernel::DataHub>();` (mandatory) or `TryFind<Interfaces::ICommandDispatcher>()` (optional). MQTT caches `weak_ptr` to avoid extending lifetime. Tests use `Test::HubLocatorInjector` (`test/unit/support/`), a `HubLocator` subclass whose ctor pre-registers all four hubs.

## 2. The hubs (`src/core/kernel/`)

`IHub` (`src/core/interfaces/ihub.h`) is a marker: virtual dtor only, no behaviour. Each hub derives from it.

- **`DataHub`** — the central read model / "current state of the pool": mode/config/board, bodies of water, temperatures + setpoints (`optional<Temperature>`), chemistry (ORP/pH/salt), the `DevicesGraph` (Boost.Graph) with typed accessors (`Auxillaries/Chlorinators/Heaters/Pumps`). **Its temperature/chemistry setters fire `ConfigUpdateSignal`** (event channel A).
- **`EquipmentHub`** — registry: `m_ActiveEquipment` (`unordered_map<type_index, unique_ptr<IEquipment>>`, one per concrete type) and `m_ActiveDevices` (`unordered_set<unique_ptr<IDevice>>`). `AddEquipment`/`AddDevice` reject null+duplicate; `DeviceExists` compares `DeviceId()`; `FindDevice(predicate)` and templated `ForEachDevice(func)` iterate. Emits `EquipmentStatusChangeSignal` (channel B).
- **`StatisticsHub`** — metrics only: `MessageCounts` (`SignallingStatsCounter`), bandwidth, latency percentiles, and serial/message-error POD sub-structs. Wired into `ProtocolTask`.
- **`PreferencesHub`** — smallest: user *display* units (`Temperature_DisplayUnits`), distinct from `DataHub::SystemTemperatureUnits()` (system/wire units).

`CommandDispatcher` is registered in the locator but is **not** an `IHub` — it's a command sink under `ICommandDispatcher`.

## 3. Status propagation — two signal channels

Both are `boost::signals2`, both synchronous (slots run inline on the one thread). Event payloads derive from `Kernel::Hub_Event` (`hub_events/hub_event.h`) which carries a `Hub_EventTypes` enum and pure virtuals `Id()` + `ToJSON()`.

- **Channel A — `DataHub::ConfigUpdateSignal`** (`shared_ptr<DataHub_ConfigEvent>`): config/telemetry. Producers: DataHub temperature/chemistry setters, and Jandy status processors firing `DataHub_ConfigEvent_ButtonStateChange` directly (`onetouch/iaq_statusprocessors.cpp`). **This is the channel actually used in production today.**
- **Channel B — `EquipmentHub::EquipmentStatusChangeSignal`** (`shared_ptr<EquipmentHub_SystemEvent>`): device/equipment status. The *designed* feed is `IStatusPublisher` (`src/core/interfaces/istatuspublisher.h`): a device's `Status(newStatus)` setter auto-fires `StatusSignal` (a `weak_ptr<const IStatus>`); `EquipmentHub::CheckAndRegisterForUpdateEvents<T>` is meant to `dynamic_pointer_cast` an added device to `IStatusPublisher`, connect a fan-in lambda (stored in `m_StatusConnections`, a `vector<scoped_connection>`) that re-emits on `EquipmentStatusChangeSignal`, guarding with `weak_ptr.lock()`.
  - ⚠️ **`CheckAndRegisterForUpdateEvents` is currently never called** (not by `AddDevice`/`AddEquipment`) — channel B's `IStatusPublisher` fan-in is **dormant infrastructure**. Teach it as the designed mechanism, but know that today device status reaches consumers via channel A. Activating it = calling it from the add path.

**Consumers** connect both channels identically: WebSocket (`src/core/http/websocket_equipment.cpp` — `ConfigUpdateSignal.connect` + `EquipmentStatusChangeSignal.connect`, each wrapping the event in `WebSocket_Event(event).Payload()` and broadcasting) and MQTT (`src/core/mqtt/mqtt_hub.cpp`). End-to-end: RS-485 message → device updates DataHub (or fires ButtonStateChange) → `ConfigUpdateSignal(event)` → every WS/MQTT slot runs **synchronously**, serialises via `ToJSON()`, publishes.

The signals are `mutable` so `const` hub refs can still emit/connect — keep that on new signals.

## 4. Threading — single-threaded cooperative

**The whole app runs on one thread.** No `std::thread`/`std::jthread` anywhere in `src/`. One `boost::asio::io_context` (a `main` local). The frame loop in `src/aqualink-automate.cpp` (~lines 428-463):

```cpp
while (!shutdown) {
    io_context.poll();                      // NON-blocking: drains ready Asio handlers
    bool had_work = protocol_task->Poll();  // RS-485 read/process/write
    if (http_server)  http_server->Poll();
    if (mqtt_integration) mqtt_integration->Poll();
    if (!had_work) std::this_thread::sleep_until(frame_start + 1ms);  // adaptive idle sleep
}
```

It uses `io_context.poll()` (returns immediately) — **not** `run()` — and each subsystem exposes a non-blocking `Poll()`. Because every hub mutation (protocol task) and read (HTTP/MQTT) happens on this one thread, **the hubs carry no locks** — documented over `EquipmentHub::ForEachDevice`. Signal slots execute inline before the emitting setter returns.

**If you add concurrency:** the `unordered_map`/`unordered_set` in `EquipmentHub` and the scalar/`optional` state in `DataHub` become data races the instant a second thread touches them. Guard them first — preferably keep all hub access on a single `boost::asio::strand` and `post()` mutations onto it, rather than sprinkling mutexes. The locator's `m_Hubs` is populated once at startup and read-only after, so runtime `Register`/`Unregister` from another thread is also unsafe.

## 5. Checklists

**Add a new hub**
1. Create `src/core/kernel/<name>_hub.{h,cpp}` deriving `Interfaces::IHub` (`~YourHub() override = default;`); follow the public-state + `mutable boost::signals2::signal<...>` layout if it emits events.
2. Add the `.cpp` to `src/core/CMakeLists.txt` (see `cmake-build-system`).
3. Register in `main` ("INFORMATION DISTRIBUTION HUBS"): `auto your_hub = std::make_shared<Kernel::YourHub>();` + `.Register(your_hub)`. Mind **ordering** if it depends on another hub at construction (like `CommandDispatcher`).
4. Add it to `test/unit/support/unit_test_hublocatorinjector.cpp` so tests resolve it.
5. Resolve in consumers via `Find<>()` (mandatory) / `TryFind<>()` (optional); cache `shared_ptr` or `weak_ptr`.
6. To expose under an interface, `Register<IYourInterface>(your_hub)`.
7. Test against the `test_kernel_hub_locator.cpp` patterns.

**Publish a new status/event**
1. Pick the channel: state/telemetry → `DataHub::ConfigUpdateSignal`; device/equipment status → `EquipmentHub::EquipmentStatusChangeSignal`.
2. Define the event under `src/core/kernel/hub_events/` deriving `Hub_Event`; implement `Id()` + `ToJSON()`; add a `Hub_EventTypes` value if needed; add the `.cpp` to CMake.
3. Emit from the producer (`hub->ConfigUpdateSignal(make_shared<YourEvent>(...))`). For channel B via `IStatusPublisher`, call `Status(newStatus)` and ensure `CheckAndRegisterForUpdateEvents` runs for that device (today you must wire that call).
4. Subscribe in WS (`websocket_equipment.cpp`) and MQTT (`mqtt_hub.cpp`); ensure their `WebSocket_Event`/`ToJSON` handling covers the new subtype.
5. Store the connection: `scoped_connection` (RAII auto-disconnect, as `m_StatusConnections`) or a plain `connection` you disconnect in teardown.
6. Update `assets/web/api/swagger.yaml` if the response JSON changed (CLAUDE.md).
7. Test that the producer fires and the payload `ToJSON()` is correct (use `HubLocatorInjector`).

## 6. Gotchas

- `Find<T>()` **throws** `Hub_NotFound`; `TryFind<T>()` returns `nullptr`. Jandy code null-checks `Find`'s result — that's dead defensive code; don't copy it. Use `Find` for mandatory deps, `TryFind` for optional.
- Resolution is by **exact static type** — register under an interface to be found by it.
- One instance per type; duplicate registration is a bug.
- **Registration order/timing**: hubs must be registered before any consumer ctor runs `Find`; cross-hub-dependent services register after their deps.
- **The single-thread invariant is load-bearing and unenforced.** Hubs have no locks; slots run inline. Any new thread / second `io_context::run()` creates silent races — add a strand/synchronisation first.
- **Signal lifetime**: a connected slot captures `this`; if the signal outlives the subscriber you get use-after-free. Always store the connection (`scoped_connection` or explicit `disconnect()`).
- `IStatusPublisher` delivers a `weak_ptr<const IStatus>` — always `.lock()` + null-check.
- `CheckAndRegisterForUpdateEvents` is dormant (never called) — adding an `IStatusPublisher` device does not auto-propagate today.
- Keep new signals `mutable`.

## Key files

`src/core/kernel/hub_locator.h`, `src/core/interfaces/{ihub,istatus,istatuspublisher,idevice,iequipment}.h`, `src/core/kernel/{data_hub,equipment_hub,statistics_hub,preferences_hub}.{h,cpp}`, `src/core/kernel/hub_events/*`, `src/core/exceptions/exception_hubnotfound.h`, `src/aqualink-automate.cpp` (registration + frame loop), `src/core/http/websocket_equipment.{h,cpp}` + `src/core/mqtt/mqtt_hub.cpp` (consumers), `test/unit/kernel/test_kernel_hub_locator.cpp`, `test/unit/support/unit_test_hublocatorinjector.*`.
