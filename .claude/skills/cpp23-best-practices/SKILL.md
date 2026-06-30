---
name: cpp23-best-practices
description: C++23 best practices and coding standards for the Aqualink-Automate project. Use when writing, reviewing, or refactoring C++ code. Covers modern idioms, safety, performance, embedded/IoT patterns, and project-specific conventions for pool automation hardware interfacing.
---

# C++23 Best Practices — Aqualink-Automate

This skill defines the coding standards, idioms, and architectural patterns for C++ development in the Aqualink-Automate project. All new code and refactors should follow these guidelines. The target standard is **C++23** (with C++20 as a fallback where compiler support is incomplete).

Subsystem-specific conventions live in dedicated skills: `jandy-protocol` (RS-485 messages), `device-navigation`, `kernel-architecture` (hubs/DI/threading), `backend-observability` (profiling/logging), `mqtt-home-assistant`, `cmake-build-system`, `webui-best-practices`, and `testing-best-practices`.

---

## 1. Core Language Preferences

### Use `std::expected` over exceptions for recoverable errors

Hardware communication is inherently fallible. Use `std::expected<T, E>` (C++23) for operations that can fail in expected ways (timeouts, bad checksums, serial read failures). Reserve exceptions for truly exceptional programmer errors.

```cpp
// GOOD — caller sees the error path clearly
std::expected<PoolStatus, AqualinkError> read_pool_status(SerialPort& port) {
    auto response = port.read(timeout_ms{500});
    if (!response)
        return std::unexpected(AqualinkError::Timeout);
    return parse_status(*response);
}

// BAD — hides failure mode
PoolStatus read_pool_status(SerialPort& port); // throws on failure
```

### Use `std::optional` for values that may not exist

```cpp
// GOOD
std::optional<float> get_temperature(SensorId id) const;

// BAD
float get_temperature(SensorId id) const; // returns -1.0f on failure
```

### Prefer `std::string_view` over `const std::string&` for read-only string parameters

```cpp
// GOOD
void log_message(std::string_view msg);

// BAD — forces allocation if caller has a C string or substring
void log_message(const std::string& msg);
```

### Use `constexpr` and `consteval` aggressively

Anything computable at compile time should be. This is especially valuable for protocol constants, CRC tables, and configuration defaults.

```cpp
constexpr std::uint8_t AQUALINK_HEADER = 0xFF;
constexpr std::uint8_t AQUALINK_ADDR   = 0x55;

consteval std::array<std::uint8_t, 256> generate_crc_table() {
    std::array<std::uint8_t, 256> table{};
    for (int i = 0; i < 256; ++i) {
        std::uint8_t crc = static_cast<std::uint8_t>(i);
        for (int j = 0; j < 8; ++j)
            crc = (crc & 1) ? (crc >> 1) ^ 0x85 : crc >> 1;
        table[i] = crc;
    }
    return table;
}

constexpr auto crc_table = generate_crc_table();
```

### Use structured bindings

```cpp
// GOOD
auto [temperature, ph, orp] = get_water_chemistry();

// BAD
auto result = get_water_chemistry();
auto temperature = std::get<0>(result);
```

### Use `enum class` with `std::to_underlying` (C++23)

```cpp
enum class PumpSpeed : std::uint8_t {
    Off    = 0x00,
    Low    = 0x01,
    Medium = 0x02,
    High   = 0x03,
};

// C++23 — clean conversion when needed for wire protocol
auto wire_byte = std::to_underlying(PumpSpeed::High); // 0x03
```

### Use `std::format` (C++20) and `std::print` (C++23) for string formatting

```cpp
// GOOD
std::print("Pool temp: {:.1f}°C  ORP: {} mV\n", temp, orp);
auto msg = std::format("Pump {} set to speed {}", pump_id, speed);

// BAD
std::cout << "Pool temp: " << std::fixed << std::setprecision(1) << temp << "°C" << std::endl;
printf("Pump %d set to speed %d", pump_id, speed);
```

---

## 2. Safety & Robustness

### Use `std::span` for buffer and array views

Never pass raw pointer + size pairs. Use `std::span` for non-owning views over contiguous data. Critical for serial protocol buffers.

```cpp
// GOOD
std::expected<Message, ParseError> parse_frame(std::span<const std::uint8_t> buffer);

// BAD
Message parse_frame(const uint8_t* data, size_t len);
```

### Prefer `std::array` over C arrays

```cpp
// GOOD
constexpr std::array<std::uint8_t, 4> probe_cmd = {0xFF, 0x00, 0x55, 0xAA};

// BAD
const uint8_t probe_cmd[] = {0xFF, 0x00, 0x55, 0xAA};
```

### Use strong types / `std::chrono` for time and durations

Never use raw integers for time. Use `std::chrono` durations throughout.

```cpp
// GOOD
using namespace std::chrono_literals;
void set_pump_runtime(std::chrono::minutes duration);
set_pump_runtime(45min);

// BAD
void set_pump_runtime(int minutes);
```

### Use RAII for all resource management

Serial ports, sockets, GPIO pins, and file handles must be wrapped in RAII classes or use smart pointers.

```cpp
class SerialPort {
public:
    explicit SerialPort(std::string_view device, BaudRate rate);
    ~SerialPort();                          // closes fd
    SerialPort(SerialPort&&) noexcept;      // moveable
    SerialPort& operator=(SerialPort&&) noexcept;
    SerialPort(const SerialPort&) = delete; // not copyable
    SerialPort& operator=(const SerialPort&) = delete;
    // ...
};
```

### Follow the Rule of Zero or the Rule of Five — never the Rule of Three

A class that does **not** directly own a resource should declare **none** of the special member functions (destructor, copy/move constructor, copy/move assignment) and let the compiler generate them — the **Rule of Zero**. Hold resources in members that already manage themselves (`std::unique_ptr`, `std::vector`, RAII wrappers) so the enclosing class needs nothing.

```cpp
// Rule of Zero — owns nothing directly, so compiler-generated members are correct.
class DeviceState {
    std::unique_ptr<Impl> m_Impl;   // member manages its own lifetime
    std::vector<Reading>  m_History;
    // no destructor, no copy/move declarations
};
```

When a class **does** directly manage a resource, declare *all five* — or `= default` / `= delete` each deliberately — the **Rule of Five** (the `SerialPort` example above). **Never use the C++98 Rule of Three** (destructor + copy constructor + copy assignment, with the move operations omitted): a user-declared destructor **suppresses the implicit move operations**, so every "move" silently degrades to a copy — a performance, and sometimes correctness, bug. In C++23 always account for the move operations.

- **Polymorphic bases** need a `virtual` (or `protected`) destructor. `virtual ~Base() = default;` is correct and *required* — it is not a Rule-of-Three offence; default the remaining members if the base must stay copyable/movable.
- **Pimpl** types must declare the destructor and define it out-of-line in the `.cpp` (where `Impl` is complete), paired with defaulted or deleted move operations as appropriate.
- Give **value types** a defaulted `operator==` (e.g. `Kernel::Temperature`, `Kernel::ORP`, `Kernel::pH`) rather than ad-hoc field comparisons, so equality lives with the type and stays consistent across callers.

### Use `[[nodiscard]]` on functions whose return value indicates success/failure

```cpp
[[nodiscard]] std::expected<AckStatus, AqualinkError> send_command(Command cmd);
```

### Use `std::unique_ptr` for exclusive ownership, `std::shared_ptr` only when truly shared

```cpp
// GOOD — clear ownership
auto device = std::make_unique<AqualinkDevice>(port_config);

// AVOID unless genuinely needed (e.g., shared between async callbacks)
auto device = std::make_shared<AqualinkDevice>(port_config);
```

---

## 3. Concurrency & Async Model

**This project is single-threaded.** There are no `std::thread`/`std::jthread` and no `io_context::run()`. One `boost::asio::io_context` is driven by a cooperative `poll()` loop in `src/aqualink-automate.cpp`:

```cpp
while (!shutdown) {
    io_context.poll();          // drain ready Asio handlers (non-blocking)
    protocol_task->Poll();      // RS-485 read/process/write
    http_server->Poll();
    mqtt_integration->Poll();
}
```

Each subsystem exposes a non-blocking `Poll()` that does bounded work per frame and returns. See the `kernel-architecture` skill for the full model. Consequences for new code:

- **Don't introduce threads.** Long-running work belongs in a subsystem `Poll()` step (do a little, return), or as an Asio async operation on the shared `io_context` — not a background `std::thread`/`jthread` and not a blocking call in the loop.
- **Shared state (the kernel hubs) is intentionally unsynchronised** — no mutexes, no `std::atomic` — because all access is on the one loop thread. `boost::signals2` slots fire synchronously, inline.
- If a multi-threaded model is ever genuinely required, introduce it deliberately (e.g. a single `boost::asio::strand` guarding hub access) and guard the affected containers first — see the note over `EquipmentHub::ForEachDevice`. Do **not** sprinkle `std::atomic`/`std::mutex` ad hoc.

---

## 4. Concepts & Templates

### Use C++20 concepts to constrain templates

Replace SFINAE and `enable_if` with concepts. Define project-level concepts for domain types.

```cpp
template<typename T>
concept Sensor = requires(T s) {
    { s.read() } -> std::convertible_to<std::optional<float>>;
    { s.id() }   -> std::convertible_to<SensorId>;
    { s.name() } -> std::convertible_to<std::string_view>;
};

template<Sensor S>
void register_sensor(S&& sensor);
```

### Use abbreviated function templates with `auto` parameters for simple cases

```cpp
// GOOD — simple generic
void log_value(std::string_view label, auto const& value) {
    std::print("{}: {}\n", label, value);
}
```

---

## 5. Ranges & Algorithms

### Use `std::ranges` and views for data pipelines

```cpp
#include <ranges>
namespace rv = std::ranges::views;

// Filter active sensors and collect readings
auto readings = sensors
    | rv::filter(&Sensor::is_active)
    | rv::transform(&Sensor::read)
    | rv::filter([](auto const& opt) { return opt.has_value(); })
    | rv::transform([](auto const& opt) { return *opt; });
```

### Use `std::ranges::to<Container>()` (C++23) to materialise ranges

```cpp
auto active_names = sensors
    | rv::filter(&Sensor::is_active)
    | rv::transform(&Sensor::name)
    | std::ranges::to<std::vector>();
```

### Prefer range-based algorithms over iterator pairs

```cpp
// GOOD
std::ranges::sort(readings, std::ranges::greater{});
auto it = std::ranges::find_if(devices, &Device::is_primary);

// BAD
std::sort(readings.begin(), readings.end(), std::greater<>{});
```

---

## 6. Error Handling Strategy

### Error type hierarchy

```cpp
enum class AqualinkError : std::uint8_t {
    // Communication
    Timeout,
    ChecksumMismatch,
    FramingError,
    ConnectionLost,

    // Protocol
    UnknownCommand,
    InvalidResponse,
    NakReceived,

    // Hardware
    SensorFault,
    PumpFault,
    OverTemperature,

    // System
    ConfigError,
    InternalError,
};

// Rich error with context
struct Error {
    AqualinkError code;
    std::string   detail;
    std::source_location location = std::source_location::current();
};
```

### Propagate errors with monadic operations on `std::expected` (C++23)

```cpp
auto result = read_frame(port)
    .and_then(validate_checksum)
    .and_then(decode_payload)
    .transform(extract_temperature);

if (!result) {
    log_error(result.error());
    return std::unexpected(result.error());
}
```

---

## 7. Project Structure & Organisation

Source-only layout — there is **no separate `include/` tree**; headers and sources live together per module. See `cmake-build-system` for how the build is wired.

```
aqualink-automate/
├── CMakeLists.txt          # root; options() before project(); version derivation
├── CMakePresets.json       # config/build/test presets per platform
├── vcpkg.json              # dependency manifest (vcpkg submodule at deps/vcpkg)
├── cmake/                  # toolchains/, vcpkg/triplets/, tools/, Sanitizers.cmake
├── src/
│   ├── core/    → libaqualink-automate  (protocol, kernel/hubs, http, mqtt, serial, logging, profiling, options)
│   ├── jandy/   → libaqualink-jandy     (messages, devices, navigation, equipment)
│   └── pentair/ → libaqualink-pentair
├── test/{unit,integration,performance}/  # Boost.Test + Google Benchmark
├── assets/web/             # Alpine.js web UI + api/swagger.yaml
└── docs/
```

### Headers, not modules

The build sets `CMAKE_CXX_SCAN_FOR_MODULES OFF` — use `#pragma once` headers. C++20 modules are deliberately **not** used (partly so clang-tidy works).

```cpp
#pragma once   // not #ifndef include guards
```

### One class per header/source pair for non-trivial types

---

## 8. Build & Tooling

Full build conventions are in the `cmake-build-system` skill. Key facts for writing code:

- **CMake 3.31+**; `CMAKE_CXX_STANDARD 23`, `CMAKE_CXX_EXTENSIONS OFF`, `CMAKE_COMPILE_WARNING_AS_ERROR ON` (warnings ARE errors), `CMAKE_EXPORT_COMPILE_COMMANDS ON`.
- **Compiler/linker/stdlib flags live in toolchain files** (`cmake/toolchains/*.toolchain.cmake`) — **never** add `$<CXX_COMPILER_ID:...>` / `$<PLATFORM_ID:...>` conditionals to a `src/` `CMakeLists.txt` (the platform-isolation rule). Per-OS source differences use plain `if(WIN32)/if(LINUX)/if(APPLE)` blocks only.
- **Sanitisers** are applied via `cmake/Sanitizers.cmake` (`target_enable_sanitizers`), driven by the `*-debug` presets. MSVC ASan needs `/MD`, so Debug+ASan uses the **Windows-LLVM** preset; sanitisers are mutually exclusive with `ENABLE_PROFILING` (configure-time FATAL).
- A **`.clang-format`** and **`.clang-tidy`** already exist at the repo root; clang-tidy runs as part of the build under `ENABLE_CLANG_TIDY` (on in the debug presets), which is why tidy advisories surface during a normal build. Match the surrounding formatting (tabs, attached braces).

---

## 9. Testing

The project uses **Boost.Test** for unit and integration tests and **Google Benchmark** for performance tests — see the `testing-best-practices` skill. A regression test MUST accompany every bug fix.

```cpp
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(JandyChecksum_TestSuite)

BOOST_AUTO_TEST_CASE(Test_Checksum_KnownValue)
{
    constexpr std::array<std::uint8_t, 3> data{ 0xFF, 0x00, 0x55 };
    BOOST_CHECK_EQUAL(CalculateChecksum(data), 0xAA);
}

BOOST_AUTO_TEST_SUITE_END()
```

### Use compile-time tests for constexpr functions

```cpp
static_assert(crc_table[0] == 0x00);
```

---

## 10. Naming Conventions

| Element             | Style              | Example                          |
|---------------------|--------------------|----------------------------------|
| Types / Classes     | `PascalCase`       | `PoolStatus`, `SerialPort`       |
| Functions / Methods | `PascalCase`       | `OnRequest()`, `IsRunning()`, `DescribeDiagnostics()` |
| Local variables     | `snake_case`       | `pool_temp`, `device_id`         |
| Constants           | `UPPER_SNAKE_CASE` | `MAX_RETRIES`, `PACKET_HEADER_LENGTH` |
| Namespaces          | `PascalCase`       | `AqualinkAutomate::Kernel`, `::HTTP` |
| Template params     | `UPPER_SNAKE_CASE` or short `PascalCase` | `MESSAGE_TYPE`, `EVENT_TYPE`, `Func` |
| Member variables    | `m_PascalCase`     | `m_EquipmentHub`, `m_PendingCommand` |
| Enum values         | `PascalCase`       | `OperatingStates::Scraping`      |
| Macros (avoid)      | `UPPER_SNAKE_CASE` | `TRACY_ENABLE`                   |

> The illustrative snippets earlier in this document prioritise the C++23 *idiom* and use simplified (often snake_case) names; **real** method and namespace names are `PascalCase` and members are `m_`-prefixed, per this table.

---

## 11. Documentation

### Use `///` Doxygen-style comments for public API

```cpp
/// Read the current pool status from the Aqualink controller.
///
/// Sends a status probe and waits for a response within the given timeout.
///
/// @param timeout Maximum time to wait for a response.
/// @return Pool status on success, or an error code on failure.
[[nodiscard]]
std::expected<PoolStatus, AqualinkError>
read_pool_status(std::chrono::milliseconds timeout = 500ms);
```

### Use `//` inline comments sparingly — code should be self-documenting

---

## 12. Embedded / IoT Specific Guidance

### Minimise dynamic allocation in hot paths

For the serial protocol handler and real-time control loops, prefer stack allocation and `std::array` over `std::vector`.

### Use `std::byte` for raw byte buffers where semantics are "just bytes"

```cpp
std::array<std::byte, 256> rx_buffer;
```

### Be explicit about integer widths

Use `<cstdint>` types (`std::uint8_t`, `std::uint16_t`, etc.) for protocol and hardware-facing code.

### Handle endianness explicitly with `std::byteswap` (C++23)

```cpp
auto temp_raw = read_uint16_be(buffer);
// If on little-endian host:
if constexpr (std::endian::native == std::endian::little) {
    temp_raw = std::byteswap(temp_raw);
}
```

### Use `std::bitset` or bit manipulation for flag registers

```cpp
constexpr auto PUMP_ON_BIT     = 0;
constexpr auto HEATER_ON_BIT   = 1;
constexpr auto SPA_MODE_BIT    = 2;

std::bitset<8> status_flags{raw_byte};
bool pump_on = status_flags.test(PUMP_ON_BIT);
```

---

## 13. Quick Reference — Prefer / Avoid

| Prefer (C++23/20)                     | Avoid                                     |
|---------------------------------------|-----------------------------------------  |
| `std::expected<T,E>`                  | Exceptions for expected failures          |
| `std::optional<T>`                    | Sentinel values (`-1`, `nullptr`)         |
| `std::format` / `std::print`         | `printf` / `iostream` formatting          |
| `std::span<T>`                        | Raw pointer + size                        |
| `std::string_view`                    | `const std::string&` for read-only        |
| Asio async on the shared `io_context` | background `std::thread`/`jthread` (project is single-threaded) |
| `std::ranges` algorithms              | Iterator-pair algorithms                  |
| `enum class` + `std::to_underlying`   | Unscoped enums                            |
| `constexpr` / `consteval`             | Runtime computation of constants          |
| Concepts                              | SFINAE / `enable_if`                      |
| `std::chrono` durations               | Raw `int` for time                        |
| `std::array`                          | C-style arrays                            |
| `std::unique_ptr`                     | `new` / `delete`                          |
| `[[nodiscard]]`                       | Silent ignored return values              |
| `std::byte`                           | `char` / `unsigned char` for raw buffers  |
| `std::byteswap`                       | Manual byte-swapping macros               |
| `std::source_location`                | `__FILE__` / `__LINE__` macros            |
| `std::ranges::to<Container>()`        | Manual `push_back` loops                  |
| Structured bindings                   | `std::get<N>(tuple)`                      |
| `#pragma once`                        | `#ifndef` include guards                  |

---

## 14. Checklist for Code Review

When reviewing or writing code, verify:

1. **No raw `new`/`delete`** — all heap allocation uses smart pointers or containers.
2. **No raw pointer+size pairs** — use `std::span`.
3. **Error paths are explicit** — `std::expected` or `std::optional`, not exceptions for expected failures.
4. **Time values use `std::chrono`** — no bare integers for durations.
5. **Public API is `[[nodiscard]]`** where return value matters.
6. **Templates are constrained** — concepts, not unconstrained `typename T`.
7. **Resources are RAII-managed** — serial ports, files, GPIO, sockets.
   - **Rule of Zero or Rule of Five, never the Rule of Three** — a class owning no resource declares no special members; one that does declares all five. A user destructor without declared move operations silently degrades moves to copies.
8. **`constexpr` where possible** — protocol constants, CRC tables, default configs.
9. **No new threads** — the app is single-threaded (one `io_context` + cooperative `poll()` loop); shared hub state is unsynchronised by design. Don't add `std::thread`/`std::jthread` or ad-hoc `std::atomic`/`std::mutex` (see `kernel-architecture`).
10. **Naming follows conventions** — see section 10.
