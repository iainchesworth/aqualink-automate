# Aqualink Automate - Claude Code Instructions

## Swagger / OpenAPI Maintenance

The API spec lives at `assets/web/api/swagger.yaml` and is served by the built-in web server at `GET /api/swagger.yaml`.

Whenever you add, modify, or remove any of the following, update `assets/web/api/swagger.yaml` to match:

- HTTP API routes (paths, methods)
- Request or response JSON schemas (fields, types, enums)
- HTTP status codes or error responses
- Query parameters, path parameters, or request bodies

## Options / Configuration

CLI flags and the optional config file are merged by a monadic pipeline (Boost.program_options) defined in `src/core/options/options_registry.h` and assembled in `src/aqualink-automate.cpp`. Each subsystem contributes an `OptionsProcessor` + a settings struct (`src/core/options/options_<area>.{h,cpp}`; subsystem options under `src/<sub>/options/`). CLI args take precedence over the config file (first-write-wins into the `variables_map`); config-file keys are the option **long names** (flat INI, no sections).

To add a CLI option / config setting:

1. Add the field to the area's `tagXxxSettings` struct and initialise it in the constructor (this is the real default fallback, since `Process()` only writes when the option `IsPresent(vm)`).
2. Declare an `AppOptionPtr` via `make_appoption(...)` with a matching `->default_value(...)`, and add it to the area's options vector.
3. In `Process()`, set the field guarded by `if (OPTION->IsPresent(vm))`. Custom value types need a `validate()` overload in the *validated type's* namespace (`src/core/options/validators/`).
4. Add conflict/dependency checks in `Validate()` if needed (`Helper_CheckForConflictingOptions` / `Helper_ValidateOptionDependencies`; both ignore defaulted options).
5. Read it via `settings.Get<Options::Xxx::XxxSettings>()`.
6. Add a test under `test/unit/options/` (the config-file key needs no extra code — it is the option long name). Only a brand-new `.cpp` (new area/validator) needs a `CMakeLists.txt` entry.
