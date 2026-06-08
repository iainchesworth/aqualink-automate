# Decoding traffic TO the master (`--decode-to-master`)

## What it is

A developer-only diagnostic that decodes and logs RS-485 frames addressed **TO** the Aqualink
master (destination `0x00`). Normally the app only decodes traffic **FROM** the master (the
master polling/addressing devices); frames sent *to* the master by other devices are parsed
but then dropped at the device-routing step (logged at debug as "AqualinkMaster not
supported"). This tool surfaces them instead.

## Observe-only — by design

This is strictly a passive observer:

- It **never transmits**, and it does **not** emulate the master.
- It does **not** replay or respond to the frames it decodes.

It only formats a decode line for analysis, so enabling it cannot perturb the bus.

## Why

Other controllers — e.g. an iAqualink2 cloud interface — send their actuation commands **TO**
the master. To reverse-engineer those commands we have to read them off the wire. Two details
make that possible:

- The tool surfaces the real **wire command byte** (`cmd=0xNN`) explicitly, because an
  unrecognised frame's message `Id` collapses to the catch-all `Unknown` (`0xff`) and would
  otherwise hide it.
- `JandyMessage_Unknown` now **retains its raw payload** (previously discarded on parse), so
  an unrecognised command's bytes are visible verbatim.

## Usage

```
aqualink-automate ... --decode-to-master --loglevel-messages info
```

Each to-master frame is logged on `Channel::Messages`, for example:

```
[to-master] cmd=0x29 len=9 chk=0x68 | Packet: Destination: AqualinkMaster (0x00), Message Type: Unknown (0xff) || Payload (2 bytes): 11 1c
```

To decode commands **offline** from a recorded capture, add `--decode-to-master` (and
`--loglevel-messages info`) to a fixture replay invocation (see `docs/RECORD_REPLAY.md` for the
replay flag set). This is how the labelled portal captures are mined for the iAqualink2's
actuation commands without touching live hardware.

## Implementation

| Piece | Location |
|-------|----------|
| Option `--decode-to-master` | `src/core/options/options_developer_options.{h,cpp}` → `DeveloperSettings::decode_to_master_enabled` |
| Gate (passed to equipment) | `src/jandy/jandy.cpp` → `JandyEquipment` ctor |
| Hook (only the `AqualinkMaster` class) | `src/jandy/equipment/jandy_equipment.cpp` `IdentifyAndAddDevice` `default:` branch |
| Formatter | `Equipment::FormatToMasterTraffic` — `src/jandy/equipment/master_traffic_snoop.{h,cpp}` |
| Raw-payload retention | `src/jandy/messages/jandy_message_unknown.{h,cpp}` (`Payload()`) |

No HTTP route is added, so the OpenAPI spec is unchanged.
