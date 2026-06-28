# CI/CD pipelines and self-hosted runners

*For maintainers operating the GitHub Actions pipelines. This is the source of truth for the workflow set, their triggers and jobs, the reusable build workflow, and self-hosted runner configuration. The release process these pipelines run is described in [docs/releasing.md](releasing.md); the branch and PR rules that drive CI triggers are in [CONTRIBUTING.md](CONTRIBUTING.md).*

## Workflow set

The pipeline is five workflow files plus two composite actions, all under `.github/`.

| File | Kind | Purpose |
|------|------|---------|
| `.github/workflows/ci.yml` | Workflow | Build, test, e2e, and Docker verification on every push and PR. |
| `.github/workflows/_build.yml` | Reusable workflow (`workflow_call`) | The shared configure/build/test/package matrix. Called by both `ci.yml` and `release.yml`. |
| `.github/workflows/release.yml` | Workflow | Build packages, publish the Docker image, and create the GitHub release for a `v*` tag. |
| `.github/workflows/automated-codescanning.yml` | Workflow | CodeQL, SonarCloud, and MSVC static analysis on PRs and a weekly cron. |
| `.github/workflows/cleanup-branch-caches.yml` | Workflow | Delete a PR's branch caches when it closes. |
| `.github/actions/setup-cpp-toolchain` | Composite action | Install the platform-appropriate compiler and build tools. |
| `.github/actions/setup-vcpkg-cache` | Composite action | Configure and restore the OS-keyed vcpkg binary cache. |

`ci.yml` and `release.yml` share one build definition (`_build.yml`), so the suites that run on a PR and the suites that gate a release can never drift.

## ci.yml

`ci.yml` runs on every push to the working branches and on PRs into the integration and release branches.

### Triggers

```yaml
on:
  push:
    branches: ["main", "develop", "feat/**", "fix/**", "docs/**", "ci/**", "test/**", "refactor/**", "chore/**", "build/**", "perf/**"]
  pull_request:
    branches: ["develop", "main"]
```

The `push` namespaces are the [allowed branch types](CONTRIBUTING.md#branch-naming) (`feat/`, `fix/`, …); a branch named outside them simply gets no push-triggered CI.

Concurrency is keyed on the PR number (or the ref for branch pushes) with `cancel-in-progress: true`, so a new push supersedes an in-flight run for the same PR or branch.

### Jobs

| Job | Runs on | What it does |
|-----|---------|--------------|
| `branch-name` | `ubuntu-latest` | PR only. Validates the PR head branch matches `<type>/<name>` with an allowed commit type, failing a non-conforming name. `develop`/`main` heads are accepted so the `develop` -> `main` release-promotion PR passes. A **required** status check on `develop`/`main`, so a misnamed branch cannot merge. |
| `build-and-test` | Per-OS matrix (see [_build.yml](#_buildyml)) | Calls `_build.yml` with no packaging. Configures, builds, and runs the full test suite on Linux, Windows, and macOS. |
| `e2e-ui` | Linux | Builds only the app binary, then runs the Playwright UI suite four times — once per mode. |
| `matter-bridge` | Linux | Node job in `matter-bridge/`: `npm ci`, typecheck (including the matter.js bridge), build, and unit tests. |
| `version-check` | `ubuntu-latest` | PR-into-`main` only. Compares what `git describe` resolves on the PR head versus the base. **Non-blocking** — it emits a `::warning::` if the version looks unchanged, never a failure. |
| `docker-verify` | Linux | Builds the `ci` and `runtime` Docker targets, smoke-tests the runtime image with `--version`, and asserts the Matter sidecar is bundled. |

**e2e-ui modes.** The four Playwright runs mirror the modes encoded in `playwright.config.ts`, selected by environment variable:

| Run | Env set | Specs exercised |
|-----|---------|-----------------|
| default (unauthenticated) | none | every spec except auth |
| auth enabled | `AQUALINK_AUTH_TOKEN` | token-gated WebSocket auth |
| history enabled | `AQUALINK_HISTORY_DB` | recorded series + chart |
| scheduler enabled | `AQUALINK_SCHEDULES_FILE` | schedule CRUD |

**docker-verify checks.** It builds the `ci` target, then the `runtime` target (tagged `aqualink-automate:verify`), runs `docker run --rm -e MATTER_ENABLED=false aqualink-automate:verify --version`, and confirms the bundled sidecar is present (`node --version` plus a test for `/opt/matter-bridge/dist/index.js`). Both Docker builds are wrapped in a bounded retry so a transient Docker Hub base-image timeout does not fail CI.

## _build.yml

`_build.yml` is the single definition of how the C++ project is configured, built, tested, and optionally packaged on each platform. It is a `workflow_call` reusable workflow — it has no triggers of its own and only runs when another workflow calls it.

### Inputs

| Input | Type | Default | Used by |
|-------|------|---------|---------|
| `do_package` | boolean | `false` | Also run `cpack`, the per-OS install/extract smoke test, and the package artifact upload. The release path sets this `true`. |
| `version_tag` | string | `""` | The resolved `v*` tag. Used only to create a local git tag for `workflow_dispatch` release builds (see below). |
| `fetch_depth` | number | `1` | Checkout depth. CI uses `1`; releases pass `0` so `git describe` and the cut-from-main check see full history and tags. |

### The matrix

The job runs a four-row matrix with `fail-fast: false`, so one platform failing does not cancel the others:

| Name | Compiler | Configure preset | Build preset | Test preset | Package preset |
|------|----------|------------------|--------------|-------------|----------------|
| Linux GCC | `gcc-15` | `config-linux-gcc` | `build-linux-gcc` | `test-linux-gcc` | `pack-linux-gcc` |
| Linux GCC (arm64) | `gcc-15` | `config-linux-gcc-arm64` | `build-linux-gcc-arm64` | `test-linux-gcc-arm64` | `pack-linux-gcc-arm64` |
| Windows MSVC | `msvc` | `config-windows-msvc` | `build-windows-msvc` | `test-windows-msvc` | `pack-windows-msvc` |
| macOS Clang | `llvm` | `config-macos-llvm` | `build-macos-llvm` | `test-macos-llvm` | `pack-macos-llvm` |

These are the same presets you use locally — see [INSTALL.md](INSTALL.md).

The arm64 row builds **natively** on an aarch64 runner (no cross-compile / QEMU) so the `.deb`/`.rpm`/`.tgz` install on a Raspberry Pi and other arm64 hosts; CPack derives the package architecture from the target, so the packages are correctly labelled `arm64`/`aarch64`. The install-tree upload (consumed by `e2e-ui` and the Docker image) stays on the x64 `config-linux-gcc` row only.

Runner selection per row is `vars.RUNNER_LINUX` / `vars.RUNNER_LINUX_ARM` / `vars.RUNNER_WINDOWS` with a GitHub-hosted fallback (the arm64 row falls back to `ubuntu-24.04-arm`); macOS always runs on `macos-latest`. See [Self-hosted runners](#self-hosted-runners).

### Test scope

The `Test` step runs the full test preset with **no `-L` label filter**, so unit and integration tests run together in one pass. There is no separate fast/slow split in CI.

### Packaging (release path only)

When `do_package` is `true`, after the test step the job also:

1. Runs `cpack --preset=<package_preset>`.
2. Smoke-tests the package on a clean target — installs the `.deb` in a fresh `ubuntu:25.04` container (Linux), extracts the ZIP and runs the exe (Windows), or extracts the TGZ and runs the binary (macOS). Each asserts `aqualink-automate --version` succeeds. This catches a missing runtime dependency before the package is published.
3. Uploads the packages as an artifact named `packages-<configure_preset>` (for example `packages-config-linux-gcc`), with `retention-days: 30`.

### Version stamping for dispatch builds

A `workflow_dispatch` release build is for a tag that does not exist yet (`github-release` pushes it at the very end). To make `git describe` stamp the right version, `_build.yml` creates a **local** tag at the start of the packaging path:

```bash
git tag "<version_tag>" HEAD   # only when do_package && event == workflow_dispatch
```

**Note:** The version is resolved through this local tag, not by passing `-DDERIVED_VERSION_OVERRIDE`. `cmake/GitVersionDerivation.cmake` does honor `DERIVED_VERSION_OVERRIDE` (it is how the Docker image is versioned, since the build context omits `.git`), but no workflow passes it during packaging.

## release.yml

`release.yml` produces a release from a version tag or a manual dispatch.

### Triggers

```yaml
on:
  push:
    tags: ["v*"]
  workflow_dispatch:
    inputs:
      version:     # e.g. v1.0.0, v1.0.0-beta.1
      prerelease:  # boolean
      dry_run:     # boolean — build packages but create no release
```

There is **no `push` to `main` trigger** — pushing to `main` does not release. A release is cut either by pushing a `v*` tag or by running the workflow manually. Concurrency is grouped on `release-<version>` with `cancel-in-progress: false`, so an in-flight release is never cancelled by a second run of the same version.

### Job graph

```
resolve-version ──> build-packages (_build.yml) ──> docker-publish ──> github-release
```

`docker-publish` and `github-release` are skipped on a dry run (`is_dry_run == 'true'`); `build-packages` still runs so the pipeline can be validated end to end without publishing.

| Job | What it does |
|-----|--------------|
| `resolve-version` | Resolves the version from the trigger, validates the tag, and enforces the release guards (below). |
| `build-packages` | Calls `_build.yml` with `do_package: true`, `fetch_depth: 0`, and the resolved `version_tag`. |
| `docker-publish` | Builds and pushes the `runtime` image, then smoke-tests the published image. |
| `github-release` | Pushes the tag for dispatch builds, downloads the package artifacts, and creates the GitHub release. |

### resolve-version guards

`resolve-version` rejects a bad release before any package is built:

- **Tag format.** The tag must match `^v[0-9]+\.[0-9]+\.[0-9]+(-(alpha|beta|rc)\.[0-9]+)?$` (for example `v1.2.3` or `v1.2.3-rc.1`).
- **Cut from main.** Real releases must be contained in `origin/main` (`git merge-base --is-ancestor` against the fetched `origin/main`). `develop` is the integration branch; only `main` holds released code. Dry runs are exempt so the pipeline can be validated from any branch.
- **Existing tag.** On a `workflow_dispatch` (non-dry-run), the job fails fast if the tag already exists, so a collision is caught before building every package rather than at the final push.

### docker-publish

This job runs only on Linux when not a dry run, with `packages: write` permission.

1. Logs in to GHCR, and to Docker Hub only if `DOCKERHUB_USERNAME` is set (so fork PRs and credential-less runs still build anonymously).
2. Derives tags from `docker/metadata-action`: `type=sha`, the full semver, `major.minor`, and a raw `latest` tag enabled **only when the release is not a prerelease**.
3. Builds and pushes the `runtime` target to GHCR (and Docker Hub when credentials are present), wrapped in a bounded retry (up to 3 attempts) so a transient Docker Hub CDN timeout does not fail the release.
4. Runs a post-push smoke test: pulls the GHCR image and asserts `--version` contains the resolved version. Because `buildx build --push` pushes straight to the registry, this genuinely exercises the published artifact rather than a local cache.

### github-release

This job runs only when not a dry run, with `contents: write` permission.

1. For a `workflow_dispatch` build, it pushes the resolved tag to `origin` at the end — the tag is created here, not at the start.
2. Downloads every `packages-*` artifact from `build-packages` (`merge-multiple: true`).
3. Runs `gh release create <tag> --generate-notes`, adding `--prerelease` when the release is a prerelease, and attaches the downloaded package files.

See [docs/releasing.md](releasing.md) for the operator-facing release walkthrough.

## automated-codescanning.yml

Code scanning is slow (each scanner does a full compile), so it runs only where code is genuinely new.

### Triggers

```yaml
on:
  pull_request:
    branches: ["develop", "main"]
  schedule:
    - cron: '42 21 * * 2'   # weekly, Tuesday 21:42 UTC
  workflow_dispatch:
```

There is **no `push` trigger.** Scanning happens on PRs into `develop` or `main`, on the weekly cron baseline of the default branch, or on demand. A former `push: [main, feature/**, bug/**]` trigger was removed because it re-scanned already-scanned code.

### Jobs

| Job | Runs on | Scanner |
|-----|---------|---------|
| `CodeScanning_CodeQL` | Linux | CodeQL (`c-cpp`). Runs its own full build, filters the SARIF to `src/**`, and uploads to the Security tab. |
| `CodeScanning_SonarCloud` | Linux | SonarCloud via build-wrapper over a coverage build (`config-linux-gcc-coverage`, `-DUSE_SONARQUBE=ON`). Runs its own full compile and uploads coverage. |
| `CodeScanning_MSVCCodeAnalysis` | Windows | MSVC code analysis (`NativeRecommendedRules.ruleset`) over the `config-windows-msvc` build; uploads SARIF. |

Each job has the same skip condition: it does not run for a `develop` -> `main` promotion PR, because that code was already scanned when it entered `develop`. Re-scanning the promotion is pure duplication.

## cleanup-branch-caches.yml

This workflow keeps the Actions cache from filling up with stale per-PR entries.

- **Trigger:** `pull_request` with `types: [closed]`.
- **Action:** On `ubuntu-latest` with `actions: write`, it installs the `gh-actions-cache` extension and deletes every cache key scoped to the closed PR's merge ref (`refs/pull/<number>/merge`). Deletion failures do not fail the workflow.

## Self-hosted runners

Every Linux and Windows job picks its runner from a repository variable, falling back to GitHub-hosted runners when the variable is unset. macOS always runs on `macos-latest` (GitHub-hosted).

### Repository variables

Set these under **Settings > Variables > Actions**. Each value is a JSON array of runner labels.

| Variable | Type | Example | Applies to |
|----------|------|---------|------------|
| `RUNNER_LINUX` | JSON label array | `["self-hosted","linux","x64"]` | x64 Linux rows of `_build.yml`, `e2e-ui`, `matter-bridge`, `docker-verify`, `docker-publish`, CodeQL, SonarCloud |
| `RUNNER_LINUX_ARM` | JSON label array | `["self-hosted","linux","arm64"]` | The arm64 Linux row of `_build.yml`. Falls back to the GitHub-hosted `ubuntu-24.04-arm` (free for public repos). |
| `RUNNER_WINDOWS` | JSON label array | `["self-hosted","windows","x64"]` | Windows row of `_build.yml`, MSVC code analysis |

### Fallback

The pattern in the workflows is `${{ fromJSON(vars.RUNNER_LINUX || '["ubuntu-latest"]') }}` (and the Windows equivalent). If the variable is unset, or the self-hosted runners go offline, jobs fall back to the GitHub-hosted runners automatically. To force the fallback, remove the repository variables.

Self-hosted jobs also run extra steps the hosted jobs skip — they clean the workspace, point vcpkg at a persistent `~/.cache/vcpkg`, and (on Windows) load the MSVC environment with the local `./.github/actions/setup-msvc-env` composite action (pure PowerShell; sources `vcvars64.bat` and exports the env delta to `$GITHUB_ENV`).

### Provisioning

Runner VM images are built with Packer under `cicd/packer/`. See [cicd/packer/README.md](https://github.com/iainchesworth/aqualink-automate/blob/main/cicd/packer/README.md) for the full provisioning, deployment, and registration procedure.

**Important:** The Architecture table in `cicd/packer/README.md` currently lists the Linux base OS as Ubuntu 24.04 / GCC 14, but the Packer template provisions **Ubuntu 25.04 with GCC 15** (`cicd/packer/linux-runner.pkr.hcl`, `cicd/packer/scripts/linux/02-gcc-toolchain.sh`). The Windows runner base is Windows Server 2022. Trust the Packer template over the table where they disagree.

## Caching

### vcpkg binary cache

The `setup-vcpkg-cache` composite action restores the vcpkg binary cache. It is keyed by runner OS, not by job:

```text
vcpkg-${{ runner.os }}-<vcpkg-commit-hash>-<hashFiles(vcpkg.json, vcpkg-configuration.json, triplets, overlays)>
```

Because OS maps 1:1 to the triplet in use, jobs that share a triplet — `build-and-test`, CodeQL, and SonarCloud all use `x64-linux-gcc` — share one cache instead of keeping four disjoint copies. The action still accepts a `cache-key-prefix` input for backward compatibility, but it is **deprecated and ignored**: it no longer affects the binary cache key.

Self-hosted runners use a persistent on-disk `~/.cache/vcpkg/archives` instead of the `actions/cache` restore, set up by the inline "Setup vcpkg cache (self-hosted)" steps.

### Compiler cache (ccache)

ccache is used inside the Docker builds (the in-Dockerfile cache mounts a `docker buildx` retry reuses) and is pre-installed on the self-hosted runner images (5 GB, compression enabled — see `cicd/packer/README.md`). The host-side CMake build steps in `_build.yml` rely on the vcpkg binary cache rather than a separate Actions-cached ccache.

### setup-cpp-toolchain behavior

The toolchain action installs only what each platform needs:

- **Linux:** installs the named compiler (`gcc-15` in CI) plus CMake and Ninja via `aminya/setup-cpp`.
- **Windows:** installs only CMake and Ninja; MSVC is assumed present (baked into the runner image).
- **macOS:** `brew install llvm cmake ninja` and prepends the Homebrew `llvm` to `PATH`, so the macOS row targets Clang 21 from Homebrew.

On self-hosted runners this action is skipped entirely — the toolchain is provisioned into the VM image.
