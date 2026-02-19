---
name: cpp23-best-practices
description: C++23 best practices and coding standards for the Aqualink-Automate project. Use when writing, reviewing, or refactoring C++ code. Covers modern idioms, safety, performance, embedded/IoT patterns, and project-specific conventions for pool automation hardware interfacing.
---

# C++23 Best Practices — Aqualink-Automate

This skill defines the coding standards, idioms, and architectural patterns for C++ development in the Aqualink-Automate project. All new code and refactors should follow these guidelines. The target standard is **C++23** (with C++20 as a fallback where compiler support is incomplete).

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

## 3. Concurrency & Async Patterns

### Use `std::jthread` with stop tokens

For background polling loops (e.g., periodic status reads), prefer `std::jthread` which joins automatically and supports cooperative cancellation.

```cpp
class StatusPoller {
    std::jthread poll_thread_;

public:
    void start(AqualinkDevice& device) {
        poll_thread_ = std::jthread([&device](std::stop_token stoken) {
            while (!stoken.stop_requested()) {
                auto status = device.poll_status();
                if (status) publish(*status);
                std::this_thread::sleep_for(5s);
            }
        });
    }
    // ~StatusPoller automatically requests stop and joins
};
```

### Use `std::atomic` for shared state, avoid mutable globals

```cpp
// GOOD
struct SharedState {
    std::atomic<float> pool_temp{0.0f};
    std::atomic<float> orp_mv{0.0f};
    std::atomic<bool>  pump_running{false};
};

// BAD
float g_pool_temp;
std::mutex g_temp_mutex;
```

### Use `std::latch` / `std::barrier` (C++20) for synchronisation rather than ad-hoc condition variables

```cpp
std::latch startup_complete{3}; // wait for 3 subsystems

// In each subsystem init:
startup_complete.count_down();

// In main:
startup_complete.wait();
std::print("All subsystems initialised\n");
```

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

### Recommended directory layout

```
aqualink-automate/
├── CMakeLists.txt
├── include/
│   └── aqualink/
│       ├── core/            # Protocol, framing, CRC
│       ├── device/          # Device abstraction, serial comms
│       ├── sensors/         # Sensor interfaces & implementations
│       ├── control/         # Pump, heater, chlorinator control
│       ├── schedule/        # Time-based scheduling (off-peak, etc.)
│       ├── integration/     # MQTT, Home Assistant, REST API
│       └── util/            # Logging, config, strong types
├── src/
│   └── (mirrors include structure)
├── tests/
│   ├── unit/
│   └── integration/
├── config/
│   └── aqualink.toml       # Runtime configuration
└── docs/
```

### Use modules where compiler support allows, otherwise conventional headers

```cpp
// If using modules (preferred when toolchain supports it)
export module aqualink.core.protocol;

export class ProtocolHandler { /* ... */ };

// Otherwise, use #pragma once (not include guards)
#pragma once
```

### One class per header/source pair for non-trivial types

---

## 8. Build & Tooling

### CMake minimum version 3.28+ for C++23 module support

```cmake
cmake_minimum_required(VERSION 3.28)
project(aqualink-automate LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
```

### Compiler warnings — treat warnings as errors in CI

```cmake
target_compile_options(aqualink PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:
        -Wall -Wextra -Wpedantic -Werror
        -Wshadow -Wnon-virtual-dtor -Wcast-align
        -Wunused -Woverloaded-virtual -Wconversion
        -Wsign-conversion -Wnull-dereference
        -Wformat=2 -Wimplicit-fallthrough
    >
)
```

### Use sanitisers during development

```cmake
# Debug builds
target_compile_options(aqualink PRIVATE
    $<$<CONFIG:Debug>:-fsanitize=address,undefined -fno-omit-frame-pointer>
)
target_link_options(aqualink PRIVATE
    $<$<CONFIG:Debug>:-fsanitize=address,undefined>
)
```

### Use `clang-format` for consistent style

Include a `.clang-format` file in the repository root. Recommended base:

```yaml
BasedOnStyle: LLVM
IndentWidth: 4
ColumnLimit: 120
AllowShortFunctionsOnASingleLine: Inline
BreakBeforeBraces: Attach
PointerAlignment: Left
IncludeBlocks: Regroup
```

### Use `clang-tidy` with a project-specific config

```yaml
# .clang-tidy
Checks: >
  -*,
  bugprone-*,
  cert-*,
  cppcoreguidelines-*,
  misc-*,
  modernize-*,
  performance-*,
  readability-*,
  -modernize-use-trailing-return-type,
  -readability-identifier-length,
  -cppcoreguidelines-avoid-magic-numbers
WarningsAsErrors: '*'
```

---

## 9. Testing

### Use a modern test framework (Catch2 v3 or GoogleTest)

```cpp
#include <catch2/catch_test_macros.hpp>

TEST_CASE("CRC calculation matches known values", "[protocol][crc]") {
    constexpr std::array<std::uint8_t, 3> data = {0xFF, 0x00, 0x55};
    REQUIRE(calculate_crc(data) == 0xAA);
}

TEST_CASE("Parse valid status frame", "[protocol][parse]") {
    auto result = parse_frame(valid_frame_bytes);
    REQUIRE(result.has_value());
    CHECK(result->pool_temp == Catch::Approx(28.5f).margin(0.1f));
}

TEST_CASE("Timeout returns expected error", "[device][serial]") {
    MockSerialPort mock_port{/* returns nothing */};
    auto result = read_pool_status(mock_port);
    REQUIRE(!result.has_value());
    CHECK(result.error() == AqualinkError::Timeout);
}
```

### Use compile-time tests for constexpr functions

```cpp
static_assert(calculate_crc({0xFF, 0x00, 0x55}) == 0xAA);
static_assert(crc_table[0] == 0x00);
```

---

## 10. Naming Conventions

| Element             | Style              | Example                        |
|---------------------|--------------------|--------------------------------|
| Types / Classes     | `PascalCase`       | `PoolStatus`, `SerialPort`     |
| Functions / Methods | `snake_case`       | `read_status()`, `send_cmd()`  |
| Variables           | `snake_case`       | `pool_temp`, `orp_reading`     |
| Constants           | `UPPER_SNAKE_CASE` | `MAX_RETRIES`, `AQUALINK_ADDR` |
| Namespaces          | `snake_case`       | `aqualink::core`               |
| Template params     | `PascalCase`       | `template<Sensor S>`           |
| Member variables    | `snake_case_`      | `device_`, `poll_interval_`    |
| Enum values         | `PascalCase`       | `PumpSpeed::High`              |
| Macros (avoid)      | `UPPER_SNAKE_CASE` | `AQUALINK_PLATFORM_LINUX`      |

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
| `std::jthread`                        | `std::thread` + manual join               |
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
8. **`constexpr` where possible** — protocol constants, CRC tables, default configs.
9. **Thread safety is explicit** — `std::atomic`, `std::jthread`, or documented single-thread assumption.
10. **Naming follows conventions** — see section 10.
