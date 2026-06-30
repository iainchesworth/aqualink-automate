# Aqualink Automate - Claude Code Instructions

## Swagger / OpenAPI Maintenance

The API spec lives at `assets/web/api/swagger.yaml` and is served by the built-in web server at `GET /api/swagger.yaml`.

Whenever you add, modify, or remove any of the following, update `assets/web/api/swagger.yaml` to match:

- HTTP API routes (paths, methods)
- Request or response JSON schemas (fields, types, enums)
- HTTP status codes or error responses
- Query parameters, path parameters, or request bodies

## Documentation Accuracy

The `docs/` tree and the root `README.md`/`CHANGELOG.md` are treated as part of the deliverable: **when you change behavior, update the doc that describes it in the same change.** A doc that contradicts the code is a defect, not just stale prose.

When you touch a subsystem, check (and update if affected) its companion doc:

| You change… | Update… |
|---|---|
| HTTP API routes, WebSocket event types, JSON request/response shapes | `assets/web/api/swagger.yaml` **and** `docs/usage-and-api.md` (route reference + WS event list) |
| CLI flags / config keys / defaults (`src/**/options/`) | `docs/configuration.md` (and `docs/mqtt-home-assistant.md`, `docs/hardware-rs485-connectivity.md`, `docs/raspberry-pi.md` for their areas) |
| Auth / TLS / bind / rate-limiting behavior | `docs/SECURITY.md` |
| GitHub Actions workflows (`.github/workflows/`), Packer/runner images (`cicd/`) | `docs/ci-cd.md`, `docs/cicd-redesign.md`, `docs/releasing.md` |
| CMake presets, build/install steps | `docs/INSTALL.md`, `docs/worktrees.md` |
| Profiling/logging facade | `docs/profiling.md` |
| Record/replay, mock harness | `docs/RECORD_REPLAY.md` |
| Matter bridge, device-ID maps | `docs/MATTER.md` |
| Jandy/Pentair wire protocol, opcodes, message types | the relevant protocol doc (`docs/to_master_decoding.md`, `docs/iaqualink2_init_handshake.md`, `docs/aqualink_rs_revisions.md`, `docs/alwin32_simulator_protocol.md`) |

Rules of thumb:

- **Prefer durable anchors over raw line numbers.** Cite symbols, function names, route URLs, option long-names, or section headers — not `file.cpp:NNN`. Bare line numbers drift the moment code is inserted above them and silently rot.
- **Design/analysis docs are dated snapshots.** `docs/async_migration_*.md` and `docs/cicd-redesign.md` are point-in-time roadmaps. Do **not** trust their file:line citations as current truth; verify against the code before relying on them, and if you reconcile one, anchor it to symbols and date the reconciliation.
- **Verify before you write.** Confirm a claim against the code (read the source, don't assume) before documenting it. If unsure, mark it explicitly as a hypothesis / pending capture rather than asserting it as fact.

## Options / Configuration

CLI flags and the optional config file are merged by a monadic pipeline (Boost.program_options) defined in `src/core/options/options_registry.h` and assembled in `src/aqualink-automate.cpp`. Each subsystem contributes an `OptionsProcessor` + a settings struct (`src/core/options/options_<area>.{h,cpp}`; subsystem options under `src/<sub>/options/`). CLI args take precedence over the config file (first-write-wins into the `variables_map`); config-file keys are the option **long names** (flat INI, no sections).

To add a CLI option / config setting:

1. Add the field to the area's `tagXxxSettings` struct and initialise it in the constructor (this is the real default fallback, since `Process()` only writes when the option `IsPresent(vm)`).
2. Declare an `AppOptionPtr` via `make_appoption(...)` with a matching `->default_value(...)`, and add it to the area's options vector.
3. In `Process()`, set the field guarded by `if (OPTION->IsPresent(vm))`. Custom value types need a `validate()` overload in the *validated type's* namespace (`src/core/options/validators/`).
4. Add conflict/dependency checks in `Validate()` if needed (`Helper_CheckForConflictingOptions` / `Helper_ValidateOptionDependencies`; both ignore defaulted options).
5. Read it via `settings.Get<Options::Xxx::XxxSettings>()`.
6. Add a test under `test/unit/options/` (the config-file key needs no extra code — it is the option long name). Only a brand-new `.cpp` (new area/validator) needs a `CMakeLists.txt` entry.
