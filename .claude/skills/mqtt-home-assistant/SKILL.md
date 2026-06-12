---
name: mqtt-home-assistant
description: MQTT and Home Assistant integration for the Aqualink-Automate project — the MqttClient/MqttHub/MqttIntegration layers, topic conventions, HA device-discovery payloads, entity types, and command-topic routing to ICommandDispatcher. Use when adding an HA entity or MQTT-published value, adding a command topic, changing discovery payloads or topics, or working on MQTT connection/lifecycle. Relevant to src/core/mqtt and the MQTT options.
---

# MQTT / Home Assistant Integration

Namespace `AqualinkAutomate::Mqtt`. Status flows out of the kernel hubs (see `kernel-architecture`) to MQTT, and HA-discovery makes those values appear as Home Assistant entities. Inbound commands route to `ICommandDispatcher`.

## Architecture — three layers (each owns the one below)

```
MqttIntegration   orchestrator; owns the hub + HomeAssistantDiscovery; wires commands to ICommandDispatcher
   └─ MqttHub      kernel-hub integration; JSON serialisation; periodic publish; command registry
        └─ MqttClient   hand-rolled MQTT 3.1.1 over TCP/TLS; poll-based; QoS-0 only
   └─ HomeAssistantDiscovery   builds + publishes the HA discovery payload (shares the MqttClient)
```

- **`MqttClient`** (`mqtt_client.*`) — **single-threaded by contract** (no async handlers/mutexes; everything advances in `Poll()` on the io_context thread), **QoS-0 only**, exponential-backoff reconnect, 60 s keepalive, publish queue capped at 1000 (drops oldest). `BuildTopic(sub)` = `"{topic_prefix}/{sub}"` (prefix default `aqualink`).
- **`MqttHub`** (`mqtt_hub.*`) — holds `weak_ptr` to Data/Equipment/Statistics hubs; serialises state to JSON; on connect subscribes `{prefix}/command/#` and publishes all status; owns the `unordered_map<string, CommandHandler>` registry and the `OnDevicesPublished` signal.
- **`MqttIntegration`** (`mqtt_integration.*`) — the only type `main` touches: `make_shared<MqttIntegration>(io_context, settings)` → `ConnectHubs(data, equipment, statistics)` (direct overload) → `Start()` → `Poll()` in the main loop.

## Topics

- **State** (published by the hub, mostly `retain=true`; full list in `mqtt_hub.h`): `status/availability` (LWT), `system/{version,equipment,status}`, `pool/{temperatures,chemistry,circulation,configuration}`, `body/{name}/temperature`, `device/{slug}` (full device JSON), `statistics/*` (**not** retained), `response/*` (command acks).
- **HA state**: `{prefix}/ha/{category}_{slug}` — a short status string, published by `HomeAssistantDiscovery` (distinct from `device/{slug}`!).
- **Commands**: hub subscribes the wildcard `{prefix}/command/#`. Builders live in `ha_discovery.cpp` (`SetpointCommandTopic`, `DeviceCommandTopic`, `ChlorinatorCommandTopic`, `CirculationCommandTopic`). Concrete: `command/setpoint/{pool,spa}`, `command/device/{slug}`, `command/chlorinator/{percentage,boost}`, `command/circulation/mode`.
- **Registry-key vs wire-topic asymmetry**: register a handler under the path *after* `command/` (e.g. key `chlorinator/percentage`); the wire topic is `{prefix}/command/chlorinator/percentage`. `ExtractCommand` strips `{prefix}/command/`. Mismatch = silent "no handler".

## HA discovery (`ha_discovery.*`)

Current format = **one device-based discovery payload** (the per-entity-topic format was migrated away). `PublishDiscoveryConfigs()` publishes ONE retained message to `{ha_discovery_prefix}/device/{ha_device_id}/config` (prefix default `homeassistant`, device id default `aqualink_aqualink`). Payload root keys:
- `dev` — device object (identifiers, name, manufacturer "Jandy / Zodiac", sw_version, model/hw_version from `DataHub.EquipmentVersions`).
- `o` — origin.
- `availability` — `[{topic: {prefix}/status/availability, payload_available:"online", payload_not_available:"offline"}]` (carried once here; components don't repeat it).
- `cmps` — keyed map of components; each has `p` (platform), `name`, `unique_id` (`UniqueId("suffix")` = `{ha_device_id}_{suffix}`), `state_topic`, `value_template`, + type-specific fields.

Entity types (builders in `ha_discovery.cpp`): `sensor` (temps, chemistry, uptime, chlorinator generating%/health), `number` slider (setpoints, chlorinator percentage), `switch` (pumps/aux/chlorinator boost), `binary_sensor` (spa/clean mode), `select` (circulation mode, dual-body only). Switch convention: `payload_on/off` = what HA sends; `state_on/off` = what HA expects on the state topic.

`Slugify` (use `HomeAssistantDiscovery::Slugify` → `Utility::Slugify`): lowercases, `[space-._]`→`_`, strips other chars. LWT/availability is set **only when `home_assistant_enabled`** (`client->SetWill("status/availability", "offline", retain)`); the app publishes "online" on connect.

## Status → publish, and commands

- **Publishing is periodic** (the real driver): `MqttHub::Poll()` fires every `status_publish_interval` (5 s) and `statistics_publish_interval` (10 s). `PublishDeviceStatus` iterates `DataHub` device collections, `Kernel::to_json`s each to `device/{slug}`, then fires `OnDevicesPublished` → HA discovery re-publish + dynamic command registration. **The signal-based `OnDataHubConfigChanged`/`OnEquipmentStatusChanged` handlers currently only `LogTrace` — they do not publish** (`publish_on_change` is wired but inert).
- **Commands**: broker → `MqttClient` PUBLISH → `MqttHub::HandleMessage` (rejects non-`command/`, `ExtractCommand` → registry key, parses JSON; plain text like `"ON"`/`"42"` is wrapped `{"raw": ...}`) → handler → `ICommandDispatcher`. The dispatcher is acquired via `TryFind<Interfaces::ICommandDispatcher>()`; handlers `weak.lock()`-check. Methods: `ToggleByLabel/Uuid`, `CommandByLabel(label, DeviceAction)`, `SetPoolSetpoint`/`SetSpaSetpoint`, `SetChlorinatorPercentage`, `SetChlorinatorBoost`, `SetCirculationMode` → `CommandResult`. Setpoint handlers convert incoming Celsius → system units before dispatch.

## Checklists

**Add an HA entity / MQTT-published value**
1. Produce the value: add to the relevant `MqttHub::Serialize*()` (for hub topics) or to `Kernel::to_json` in `json_data_hub.cpp` (for per-device `device/{slug}` JSON).
2. Confirm it's published — reusing an existing state topic needs no new publish call (periodic timer covers it); a new topic needs a `m_Client->Publish(BuildTopic("..."), payload, true)` + a doc line in `mqtt_hub.h`.
3. Add the HA discovery component in the matching `Add*Components` builder: `cmps[key] = {…}` with required `p`, `name`, `unique_id` (`UniqueId(...)`), `state_topic`, `value_template`, + type-specific keys. Platform must be `sensor|binary_sensor|number|switch|select`.
4. Writable entity (number/switch/select) → also do the command-topic checklist.
5. Tests: update `test/unit/mqtt/test_mqtt_ha_discovery.cpp` (it asserts component counts).
6. Swagger: only if you touched `json_data_hub.cpp` (which also feeds the REST `/api` device JSON) — then update `assets/web/api/swagger.yaml`. MQTT topics themselves are not in the OpenAPI spec.

**Add a command topic**
1. Add the method to `Interfaces::ICommandDispatcher` (+ implement in the concrete dispatcher) returning a `CommandResult`.
2. Register the handler in `mqtt_integration.cpp` at the right stage (`RegisterDefaultCommands` / post-hub / dynamic `RegisterDynamicDeviceCommands` with a `HasCommand` guard). **Registry key = path after `command/`.** Capture `weak_ptr<ICommandDispatcher>`, `.lock()`-check, parse with `ParsePayloadNumber`/`ParsePayloadString`. No new subscription needed (`command/#` covers it).
3. HA-driven → add a command-topic helper in `ha_discovery.cpp` and set `command_topic` on the component.
4. Test in `test_mqtt_integration.cpp`.

## Gotchas

- **Two per-device state topics**: hub publishes full JSON to `device/{slug}`; HA discovery publishes a short string to `ha/{category}_{slug}`. Components mix them (chlorinator switch reads `ha/...`; its %/boost/health read `device/...`). Point `state_topic` at the right one or the entity shows "unavailable".
- **Registry key ≠ wire topic** (strip `{prefix}/command/`).
- **`publish_on_change` is inert** — only periodic timers publish; a config change does not trigger an immediate push.
- **`OnDevicesPublished` fires every ~5 s** and re-runs full discovery + dynamic command registration (idempotent only via `HasCommand` guards) — don't add non-idempotent work there.
- **LWT/availability only when HA enabled** — MQTT-without-HA never publishes `status/availability`.
- **`m_CommandDispatcher` is set only by the `ConnectHubs(HubLocator&)` overload**; `main` uses the direct three-hub overload, so verify the dispatcher is actually wired before assuming commands reach hardware.
- **`MqttClient` is single-threaded + QoS-0** — no cross-thread calls, no delivery guarantee, oldest-dropped past 1000 queued.
- **Setpoint unit conversion lives in the handler** (Celsius→system units) — replicate for any new temperature command.
- **Two slugifiers** — use `HomeAssistantDiscovery::Slugify` for entity/topic slugs (the `ha_device_id` derivation in options is a separate inline slug that also handles `/`).
- Discovery **component counts are asserted** in tests — update them when adding/removing fixed components.

## Key files

`src/core/mqtt/{mqtt_client,mqtt_hub,mqtt_integration,ha_discovery}.{h,cpp}`, MQTT options `src/core/options/options_mqtt_options.*`, `src/core/interfaces/icommanddispatcher.h`, `src/core/utility/slugify.h`, `src/core/http/json/json_data_hub.cpp` (`to_json` device payload), `src/aqualink-automate.cpp` (construction/Poll), tests `test/unit/mqtt/*`.
