---
name: jandy-protocol
description: RS-485 Jandy/Zodiac serial protocol engineering for the Aqualink-Automate project. Use when adding or modifying message types, working with serialize/deserialize, the static message signal dispatch, checksums/framing/DLE-null escaping, device identification, the message generator and factory, the protocol read/write engine, or serial-port implementations. Covers the end-to-end "add a new wire message" workflow and the common pitfalls. Use for anything under src/jandy/messages, src/jandy/generator, src/jandy/factories, src/core/protocol, or src/core/serial.
---

# Jandy/Zodiac RS-485 Protocol — Engineering Patterns

How the message/protocol layer is *built*. Byte-level protocol facts (what each message means on the wire) live in the project memory (`iaq_protocol.md`, `iaq_chlorinator_commands.md`, `chlorinator_analysis.md`) and `docs/hardware-rs485-connectivity.md` — this skill is about the **code shape**: how to add and change message types correctly.

Related: device behaviour and screen-scraping live in `device-navigation`; the single-threaded execution model and hubs live in `kernel-architecture`.

## Architecture at a glance

Bytes flow `SerialPort → circular buffer → generator → factory → message object → static per-type signal → device slot`. Replies flow back `device → message->Signal_MessageToSend() → ProtocolTask write signal → Serialize → write queue`. Everything runs on **one thread** (the `ProtocolTask::Poll()` loop) — see `kernel-architecture`.

## 1. Message anatomy — the template-method rule

Interfaces in `src/core/interfaces/`: `IMessage<ID_ENUM>` (`imessage.h`), `ISerializable` (`iserializable.h`), `IMessageSignalRecv<T>` / `IMessageSignalSend<T>`.

The shared engine is `JandyMessage` (`src/jandy/messages/jandy_message.{h,cpp}`). It is the single most important pattern:

- **`Serialize()` / `Deserialize()` are `final` on the base.** They own framing (DLE/STX … checksum/DLE/ETX), checksum calc, DLE-null escaping, validation, and — on deserialize — re-resolving the id enum from the wire byte via `magic_enum::enum_cast` + `SetId()`.
- **Subclasses implement only `SerializeContents(std::vector<uint8_t>&)` and `DeserializeContents(std::span<const uint8_t>)`** — payload only. **Never** add header/checksum/footer in a subclass; that double-frames the packet.
- Framing offsets are named constants on the base: `Index_DataStart{4}`, `PACKET_HEADER_LENGTH{4}`, `PACKET_FOOTER_LENGTH{3}`, `MIN`/`MAXIMUM_PACKET_LENGTH`.

Each device **family** has an intermediate abstract base (`AquariteMessage`, `IAQMessage`, `PDAMessage`, `SerialAdapterMessage`, …) that derives `JandyMessage`, re-declares the two `*Contents` methods pure, and supplies a family `ToString()`. New messages derive the family base, not `JandyMessage` directly.

Conventions every message follows (see `jandy_message_ack.{h,cpp}`, `aquarite/aquarite_message_percent.{h,cpp}`, `iaq/iaq_message_heartbeat.{h,cpp}`):
- `static constexpr JandyMessageIds ID = JandyMessageIds::Xxx;`
- A `noexcept` default constructor that chains the family base with that id. **The factory concept requires `noexcept` default-constructibility** — omit it and you get a cryptic constraint error.
- Payload accessors expose *domain meaning* (`GeneratingPercentage()`, `IsBoostMode()`), backed by named `Index_*` / `Value_*` constants — not raw byte getters.
- **`Index_*` are absolute offsets into the full framed packet and start at 4** (`Index_DataStart`), because `DeserializeContents` receives the whole packet span. Off-by-four is the classic bug here.

## 2. Dispatch — static per-type signals

`IMessageSignalRecv<T>` holds **one `static` signal per concrete message C++ type** (`GetSignal()` → `shared_ptr<boost::signals2::signal<void(const T&)>>`). A parsed message is upcast to `IMessageSignalRecvBase*` and `Signal_MessageWasReceived()` (virtual) `dynamic_cast`s back to `T` and fires that type's signal. Send side mirrors this: `IMessageSignalSend<T>::GetPublisher()` + `Signal_MessageToSend()`.

Because the signal is **global per type, not per instance**, every device of a class shares one signal. Per-device handlers therefore filter by id:

- `SlotConnectionManager` (`src/jandy/utility/slot_connection_manager.h`) — `RegisterSlot<T>(handler)` (unfiltered) and `RegisterSlot_FilterByDeviceId<T>(handler, JandyDeviceId)` which wraps the slot in a `FilteredSlot_ByDeviceId<T>` (`filtered_slot.h`) that drops any message whose `Destination().Id()` ≠ the registered id. Connections auto-disconnect on destruction.

**Always** filter per-device handlers by `DeviceId().Id()`, or every instance handles every device's traffic.

## 3. Bytes ↔ message (generator + factory)

- **Generator** `src/jandy/generator/jandy_message_generator.{h,cpp}`: `GenerateMessageFromRawData(circular_buffer<uint8_t>&)` returns `std::expected<shared_ptr<JandyMessage>, error_code>` (codes in `errors/jandy_errors_protocol.h` — never exceptions). It finds packet bounds, cleans stale bytes, validates checksum, then calls the factory. `jandy_rawdata_generator.cpp` (`GenerateRawDataFromMessage`) is a **dead stub** — encoding is `JandyMessage::Serialize()`; don't add send logic there.
- **Factory** `src/jandy/factories/jandy_message_factory.h`: a `consteval` perfect-dispatch table (4-slot hot cache + 256-slot cold table keyed by `magic_enum::enum_integer(id)`). `CreateFromSerialData(range)` reads the type byte at index 3, creates the instance, and deserializes. The `.cpp` files are intentionally empty — everything is compile-time.
- The type→id registration list is `JandyMessageFactoryT` in `src/jandy/factories/jandy_message_factory_registration.h` (`REGISTER_MESSAGE(Type, Id)` / `REGISTER_HOT_MESSAGE` — hot capacity is only 4: `Ack`, `Status`, `Probe`). **Forgetting to register = silent `Unknown`**: no compile error, bytes are dropped.

Multi-protocol seam: `MessageGeneratorRegistry` (`src/core/protocol/message_generator_registry.{h,cpp}`) holds priority-ordered generators; Jandy registers in `src/jandy/protocol/jandy_protocol_registration.cpp`. A `ProtocolMessageWrapper` type-erases the message so the core engine never names `JandyMessage`.

## 4. Checksum, framing, DLE-null

- Frame markers in `jandy_message_constants.h`: `DLE=0x10`, `STX=0x02`, `ETX=0x03`. Layout: `DLE STX | dest | type | payload… | checksum | DLE ETX`.
- Checksum (`src/jandy/utility/jandy_checksum.h`, header-only): sum bytes into `uint32_t`, take low 8 bits, computed over everything before the checksum byte (`size - PACKET_FOOTER_LENGTH`).
- DLE-null escaping (`src/jandy/utility/jandy_null_handler.h`): literal `0x10` payload bytes are escaped `0x10 0x00`. The base handles this; the fast deserialize path skips copying when no escapes are present (and relies on the buffer being **linearised** first).

## 5. Device identity (`src/jandy/devices/`)

- `JandyDeviceId` (`jandy_device_id.h`): a `uint8_t` value wrapper; `operator()()` returns the raw byte; has a `std::hash`.
- `JandyDeviceType` (`jandy_device_types.h`): maps a raw id to a `DeviceClasses` enum via a hard-coded id→range table (`AqualinkMaster 0x00–03`, `AqualinkTouch 0x30–33`, `OneTouch 0x40–43`, `SWG_Aquarite 0x50–53`, `IAQ 0xA0–A3`, …). `message.Destination().Class()` is the switch key for device creation.

## 6. Protocol engine & serial

- `ProtocolTask` (`src/core/protocol/protocol_thread.{h,cpp}`; `protocol_handler.h` is just a shim). `Poll()` each frame: `DrainWrites()` → `DrainReads()` (into `m_SerialBuffer` circular buffer) → if data: `linearize()` → `ProcessMessages()` → `DrainWrites()` again (so a reply ships in the same frame). Returns `bool` (work-done) so the main loop can skip sleeping.
- `ProcessMessages()` loops the registry generator; on success calls `ProtocolHandler_ReadOp_MessageHandler` which just calls `Signal_MessageWasReceived()`. It only **continues** the parse loop for recoverable codes (`DataAvailableToProcess`, `ChecksumFailure`, `OverlappingPackets`) — a new recoverable code must be added to that allow-list or parsing stalls.
- Write side: `ProtocolTask::ConnectWriteSignal<T>()` connects `T::GetPublisher()`, serializes, and enqueues. Wired in `src/aqualink-automate.cpp` for `JandyMessage_Ack` and `IAQMessage_ControlDataResponse`.
- Serial abstraction: `ISerialPortImpl` (`src/core/interfaces/iserialportimpl.h`); concrete `SerialPort` wrapper owns a `unique_ptr<ISerialPortImpl>`. Impls: physical (`port_types/physical_serial_port_impl`), network/TCP with RFC2217-vs-raw strategy (`port_types/network_serial_port_impl` + `serial/rfc2217/`), mock replay (`developer/mock_serial_port_impl`), and recording **decorator** (`developer/recording_serial_port_impl`). Selection is in `aqualink-automate.cpp`.

## 7. Wiring (`src/jandy/equipment/jandy_equipment.cpp`)

- **Discovery**: the `JandyEquipment` ctor connects every message type's signal to `IdentifyAndAddDevice`, which `switch`es on `Destination().Class()` and `AddDevice()`s the matching device (real devices created with `is_emulated = false`; emulated ones come from `src/jandy/jandy.cpp` with `true`).
- **Per-device handling**: each device ctor calls `m_SlotManager.RegisterSlot_FilterByDeviceId<MsgT>(std::bind(&Dev::Slot_X, this, _1), DeviceId().Id())`. Emulated send goes through the `Emulated` capability (`devices/capabilities/emulated.h`), which builds an `Ack` and calls `Signal_MessageToSend()` — but only when `IsEmulated()`.

## 8. Checklist — add a new message type end-to-end

For a receive-only `FooMessage_Bar` (wire id `0x99`, family `foo`):

1. **Id**: add `Foo_Bar = 0x99` to `src/jandy/messages/jandy_message_ids.h` (range `0x00–0xFF` already covered by `magic_enum` customization).
2. **Family base** (if new): `src/jandy/messages/foo/foo_message.{h,cpp}` deriving `JandyMessage`, `*Contents` pure, family `ToString()`.
3. **Message class**: `src/jandy/messages/foo/foo_message_bar.{h,cpp}` — derive the family base + `IMessageSignalRecv<FooMessage_Bar>` (add `IMessageSignalSend<>` only if sent); `static constexpr ID`; `noexcept` default ctor; `Index_*`/`Value_*` constants (remember: absolute, start at 4); implement `ToString`, `SerializeContents`, `DeserializeContents` (length-check first; log on `Channel::Messages`); domain accessors.
4. **Build**: add both `.cpp`s to `src/jandy/CMakeLists.txt` `target_sources` (and the family base if new) — see the `cmake-build-system` skill.
5. **Register in factory**: in `jandy_message_factory_registration.h` add the `#include` and a `REGISTER_MESSAGE(Messages::FooMessage_Bar, Messages::JandyMessageIds::Foo_Bar)` line (`REGISTER_HOT_MESSAGE` only if genuinely high-frequency).
6. **Discovery wiring** (only if seeing this message should instantiate a device): add the `GetSignal()->connect(... IdentifyAndAddDevice ...)` line in `jandy_equipment.cpp`.
7. **Consume in the device**: `RegisterSlot_FilterByDeviceId<FooMessage_Bar>(...)` in the owning `*_device.cpp` ctor + a `Slot_Foo_Bar` handler declared in the device header.
8. **Sendable only**: `protocol_task->ConnectWriteSignal<Messages::FooMessage_Bar>();` in `aqualink-automate.cpp`, and emit via `msg->Signal_MessageToSend()`.
9. **Formatter** (if needed): `std::formatter` specialisations in `src/jandy/formatters/jandy_message_formatters.{h,cpp}`.
10. **Test** (mandatory): `test/unit/messages/test_foo_message_bar.cpp` modelled on `test_message_ack.cpp` — construction, accessors, and a `Serialize`→`Deserialize` round-trip. Register the file in `test/CMakeLists.txt`.
11. **Swagger**: update `assets/web/api/swagger.yaml` only if the change surfaces in the HTTP API (a pure wire message usually does not).

## 9. Gotchas

- `Index_*` are absolute (start at 4), not payload-relative.
- `SerializeContents` must not frame/checksum — the `final` base does it.
- `MessageLength`/`ChecksumValue`/`RawId` are only valid after deserialize; deserialize also rewrites `Id()` from the wire byte.
- The static signal is per C++ type, not per instance — filter per-device by `DeviceId().Id()`.
- Skipping factory registration = silent `Unknown`, dropped bytes, no compile error.
- The factory concept requires a `noexcept` default ctor.
- Hot-path table holds only 4; extra `REGISTER_HOT_MESSAGE` overflow silently falls to the cold table (perf, not correctness).
- A new recoverable generator error code must be added to the `ProcessMessages` continue-list or parsing stalls.
- The buffer must be `linearize()`d before span deserialize (the zero-copy path is UB on a wrapped circular buffer).
- Use the right log channel: `Channel::Messages` (message code), `Channel::Signals` (dispatch), `Channel::Protocol` (engine), `Channel::Devices`/`Equipment` (wiring).
- `jandy_rawdata_generator.cpp` and `GenerateRawDataFromMessage` are dead stubs.

## Key files

- Interfaces: `src/core/interfaces/{imessage,iserializable,imessagesignal_recv,imessagesignal_send,ideviceidentifier,iserialportimpl}.h`
- Base/ids/constants: `src/jandy/messages/{jandy_message.{h,cpp},jandy_message_ids.h,jandy_message_constants.h}`
- Examples: `src/jandy/messages/jandy_message_ack.{h,cpp}`, `aquarite/aquarite_message_percent.{h,cpp}`, `iaq/iaq_message_heartbeat.{h,cpp}`
- Dispatch/filter: `src/jandy/utility/{slot_connection_manager.h,filtered_slot.h}`
- Generator/factory: `src/jandy/generator/jandy_message_generator*.{h,cpp}`, `src/jandy/factories/jandy_message_factory{,_registration}.h`, `src/jandy/concepts/jandy_message_type.h`
- Checksum/escaping: `src/jandy/utility/{jandy_checksum,jandy_null_handler}.h`
- Identity: `src/jandy/devices/{jandy_device_id,jandy_device_types}.h`
- Engine/serial: `src/core/protocol/{protocol_thread.{h,cpp},message_generator_registry.{h,cpp},protocol_message_types.h}`, `src/jandy/protocol/jandy_protocol_registration.cpp`, `src/core/serial/**`, `src/core/developer/{mock,recording}_serial_port_impl.{h,cpp}`
- Wiring: `src/jandy/equipment/jandy_equipment.cpp`, `src/aqualink-automate.cpp`
- Tests: `test/unit/messages/test_message_ack.cpp`
