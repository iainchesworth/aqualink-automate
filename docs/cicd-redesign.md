# CI/CD Redesign — Build-Once → Promote

> Produced by a multi-agent analysis of the GitHub CI / release / packaging / tagging flow
> (2026-06-15). The **plan** (synthesis) is followed by an **adversarial critique** that corrects
> several over-claims. Read both — the critique's MUST-FIX items qualify the plan.

## TL;DR — recommended order

- **Ship now (pure wins, low risk):** CTest LABELS (Phase 0) and a triplet/OS-keyed vcpkg binary
  cache (Phase 1). The critique calls the cache re-keying *"the highest-true-value change in the plan."*
  Both are implemented in the same PR as this doc.
- **The "13→3 builds" headline is overstated** — honest count is ~13→7 for a release (3 builds +
  SonarCloud coverage + CodeQL + MSVC analysis + matter TypeScript all still compile). The genuine
  prize is eliminating duplicate **vcpkg** builds via one shared cache, not eliminating compiles.
- **Two plan pillars are wrong as written** and need the critique's fixes first: "assembly-only
  Docker" rests on an undocumented runner-OS == Docker-base ABI coincidence (breaks on the
  `ubuntu-latest` fallback), and scanners **cannot** consume prebuilt CMake build trees (CodeQL /
  SonarCloud / MSVC analysis must observe a live compile).

---

# Plan (synthesis)

## 1. The core problem

For a **single release**, the C++ app and its full vcpkg dependency set are compiled from cold
roughly **13 times** across jobs that share zero artifacts: `ci.yml build-and-test` (×3
Linux/Windows/macOS) + `ci.yml e2e-ui` (rebuilds the Linux app) + `ci.yml docker-verify` (rebuilds
vcpkg+app inside the Docker `ci` stage) + `automated-codescanning.yml` (CodeQL, SonarCloud-coverage,
MSVC-analysis = ×3) + `release.yml build-packages` (×3 again) + `release.yml docker-publish`
(rebuilds the `ci` stage from scratch inside Docker). Root causes are structural: **no build
artifact ever crosses a job or workflow boundary** (every job re-runs `configure → vcpkg → build`),
the Docker `runtime` stage does `COPY --from=ci /src/install/config-linux-gcc/` so the `ci` stage
recompiles everything (`Dockerfile:186` / `:100-133`), and the three CTest cases —
`AqualinkAutomateUnitTests` (`test/CMakeLists.txt:223`), `AqualinkAutomateIntegrationTests` (`:354`),
`AqualinkAutomatePerfTests` (`:407`) — carry **no labels**, so `ctest --preset test-linux-gcc` runs
the slow integration binary in *both* CI and release with no way to gate it on a PR. Consequences
seen this week: hours-long releases; Docker Hub CDN/rate-limit flakiness; self-hosted runner
saturation (a beta stuck `queued` ~2h); and a real beta that **failed at `ctest` during
`release.yml`** because the author never ran the integration target locally and the failure first
surfaced *after* packaging. Tagging is fully manual (admin-merge develop→main, hand-push `v*`),
leaving orphaned beta tags.

**Base design chosen:** "build-once-promote" as the spine — its `workflow_call` reusable build,
`DERIVED_VERSION_OVERRIDE` threading, and triplet-keyed cache are the strongest, lowest-risk core.
Grafted in: **CTest LABELS** (Phase 0, cheapest highest-value fix), an explicit `.dockerignore`
carve-out + dedicated Docker context, and a **concurrency guard excluding `main`/`develop`**.
Deliberately **deferred** (scope-creep / dual-source-of-truth risk): release-please / auto-tag-on-merge
and a checked-in `VERSION` file — kept as an optional, last, opt-in phase.

## 2. Target architecture

### Workflow / job graph

```
_build.yml  (NEW — workflow_call; THE ONLY PLACE A COMPILER RUNS)
 └─ build  [matrix: linux-gcc / windows-msvc / macos-llvm]      runs-on: RUNNER_HEAVY (self-hosted)
      checkout(submodules,fetch-depth:0) → setup-runner → setup-vcpkg-cache
      → configure config-<plat>  (-DDERIVED_VERSION_OVERRIDE=${{inputs.version_override}})
      → build build-<plat>  (app + test targets)
      → ctest test-<plat> -L unit         (always)
      → ctest test-<plat> -L integration  (if inputs.run_integration)
      → cmake --install build/config-<plat>
      → cpack pack-<plat>   (if inputs.do_package)
      uploads:  installtree-<plat>  (always),  buildtree-<plat>  (if upload_buildtree),
                packages-config-<plat> (if do_package),  test-results-<plat>

ci.yml      (PR → develop/main; push → develop)        concurrency: cancel PRs only, never main/develop
 ├─ build         uses: ./_build.yml   run_integration=true, upload_buildtree=(linux,windows)
 ├─ e2e-ui        needs build → download installtree-linux-gcc → AQUALINK_EXE=… → Playwright ×4   [LIGHT]
 ├─ docker-verify needs build → download installtree-linux-gcc → docker build --target runtime    [LIGHT]
 ├─ scan-linux    needs build → CodeQL + SonarCloud (shares vcpkg cache; still compiles)          [HEAVY*]
 ├─ scan-windows  needs build → msvc-code-analysis-action (shares vcpkg cache; still compiles)     [LIGHT]
 ├─ matter-bridge (Node-only, independent)                                                         [LIGHT]
 └─ version-check (PR→main only, BLOCKING)                                                         [LIGHT]

release.yml (push tag v*; or workflow_dispatch)        concurrency: per-version, cancel-in-progress:false
 ├─ resolve-version  validate tag, cut-from-main guard, existing-tag guard
 ├─ build            uses: ./_build.yml  with: version_override=<tag>, do_package=true, run_integration=true
 ├─ docker-publish   needs [resolve-version, build] → download installtree-linux-gcc
 │                    → docker build --target runtime --push (no C++ recompile; matter TS still builds)
 └─ github-release   needs [resolve-version, build, docker-publish]
                      → download packages-config-* → gh release create → (dispatch) push tag LAST

automated-codescanning.yml  →  DELETED  (jobs absorbed into ci.yml scan-linux/scan-windows; weekly cron moves there)
```
\* `scan-linux` still compiles (SonarCloud needs a `config-linux-gcc-coverage` build; CodeQL must
observe a compile) — see §5 and critique #3/#4.

### Build-once artifact data-flow (PR → main → tag → release)

```
PR (→develop/→main): ci.yml runs _build.yml ONCE (×3). e2e/docker consume artifacts.
   Required checks: build(×3), integration tests (in build), e2e-ui, docker-verify, scan-linux, scan-windows.
        ▼ merge develop → main (admin/merge-queue; version-check BLOCKING on →main)
   Tag push  v<M>.<M>.<P>[-(alpha|beta|rc).<N>]   (manual today; optional auto-tag deferred)
        ▼
   release.yml: resolve-version → _build.yml(version_override=tag, do_package) → docker-publish → github-release
```

The "build once" boundary is the `build` node **inside each workflow run** — within a single
release run the bits packaged into `.deb/.rpm/.tgz/.zip/.exe/.dmg`, the install tree that becomes the
Docker image, and the test results are the *same compiled objects*. Honest scope: reuse is
**per-run, not cross-run** — `release.yml` still runs `_build.yml` once because the SHA differs after
the develop→main merge.

## 3. Concrete per-file changes

- **`test/CMakeLists.txt` (Phase 0, ship first):** add `set_tests_properties(... LABELS unit|integration|perf)`
  after the three `add_test` calls. Enables `ctest --preset <p> -L unit` (fast) vs `-L integration`.
  Pure CMake, locally verifiable, zero workflow risk. *(Caveat: labels only help once the ctest
  invocation passes `-L`; the presets have no filter, so add the `-L` to the ctest step.)*
- **`.github/workflows/_build.yml` (NEW):** `workflow_call` with inputs
  `platform, configure_preset, build_preset, test_preset, package_preset, version_override, run_integration, do_package, upload_buildtree`.
  One matrix job: configure → build → `ctest -L unit` → `ctest -L integration` (gated) → install →
  cpack (gated) → smoke (gated) → uploads. Threads `-DDERIVED_VERSION_OVERRIDE`.
- **`ci.yml` (rewritten):** `build-and-test` becomes a one-line `uses: ./_build.yml`; `e2e-ui` and
  `docker-verify` drop their builds and download `installtree-linux-gcc`; scanners absorbed; add the
  concurrency guard excluding `main`/`develop`.
- **`release.yml` (rewritten):** `build-packages` → `uses: ./_build.yml` (same reusable build as CI,
  so integration can't diverge); `docker-publish` consumes the install artifact; `github-release`
  pushes the tag only after docker+packages succeed, with an `if: failure()` finalizer that deletes a
  *prerelease* tag whose release didn't complete (kills orphaned betas).
- **`Dockerfile` + `.dockerignore`:** add an assembly-only `runtime` path that `COPY`s a pre-built
  install tree from a dedicated, un-ignored `docker/context/` dir (`.dockerignore` adds `!docker/context/`).
  Keep the from-source `ci` stage for local/dev + a scheduled cold-build check. *(See critique #1/#2.)*
- **`cmake/CPackConfig.cmake` / `cmake/GitVersionDerivation.cmake` / `CMakePresets.json`:** unchanged.
- **`.github/actions/setup-vcpkg-cache/action.yml` (Phase 1):** re-key the binary cache on
  triplet/runner-OS, dropping the per-job `cache-key-prefix` (which produced 4 disjoint
  `x64-linux-gcc` caches). Coverage and release legitimately share vcpkg binaries.

## 4. Caching + reliability

- **vcpkg binaries** — `actions/cache`, OS/triplet-keyed, restored only inside the build job; one
  cache per triplet shared across build + coverage scan.
- **Docker / BuildKit** — runtime build assembly-only on the hot path; add `type=gha` buildx cache so
  apt + matter `npm ci` layers survive retries/eviction.
- **`cleanup-branch-caches.yml`** — stop deleting the content-addressed vcpkg caches on PR close.
- **Retries scoped to network only** (`nick-fields/retry` around Docker pulls + vcpkg bootstrap);
  **never** retry compile/ctest. `fail-fast: false` on the build matrix.
- **Integration tests become a blocking PR gate**, run by the same `_build.yml` the release uses —
  the exact beta-killer fix.
- **Runner split** `RUNNER_HEAVY` (compile + coverage scan) vs `RUNNER_LIGHT` (e2e/docker/release/
  scan-windows) — peak heavy jobs per commit drop from ~6 to 3.

## 5. Security-scan placement

Delete `automated-codescanning.yml`; fold into `ci.yml` as `scan-linux` (CodeQL + SonarCloud) and
`scan-windows` (MSVC analysis). Drop the noisy `push: feature/**, bug/**` triggers; scan on
`pull_request: [develop, main]` + `push: [main]` + the weekly cron; make CodeQL + SonarCloud required
PR checks. **They still compile** (see critique #3/#4) — the win is sharing the triplet-keyed vcpkg
cache and consolidating runners, not eliminating the compile.

## 6. Phased migration (each phase independently shippable & green)

| Phase | Change | Risk |
|---|---|---|
| **0** | `test/CMakeLists.txt` LABELS; `docs/releasing.md` `ctest -L` note. Pure CMake. **Ship now.** | very low |
| **1** | OS/triplet-key the vcpkg binary cache; narrow `cleanup-branch-caches.yml`. | low |
| **2** | Extract `_build.yml` (`workflow_call`); `ci`/`release` call it (compile count unchanged — one build *definition*). | medium |
| **3** | Assembly-only Docker (`!docker/context/` carve-out; `type=gha` cache); re-prove the smoke tests. | medium |
| **4** | Consumers stop compiling: `e2e-ui`/`docker-verify` download artifacts; fold scanners; delete codescanning; narrow triggers. | low–med |
| **5** | Versioning hardening: thread `version_override`; shared parse-version action; `version-check` blocking; orphan-tag finalizer; tag-after-success; concurrency guard; required checks. | medium |
| **6 (optional)** | Auto-tag-on-merge + `gh release create --notes-file`. Opt-in; manual hand-push stays. **Defer.** | medium-high |

## 7. Trade-offs & risks (plan's own list — see critique for corrections)

- CodeQL `build-mode:none` is unvalidated against this repo's Boost/template load.
- Reuse is per-run, not per-commit (release re-builds the merge commit).
- Reduced cold-path coverage once docker/e2e/release consume artifacts (mitigate with a scheduled
  — better, per-release — cold build gate).
- `.dockerignore` `!docker/context/` carve-out is a footgun (empty image if wrong).
- Required-check hardening trades release-time flakiness for merge-time flakiness — sequence it after
  the retry/cache work.

---

# Adversarial critique

Verified against the repo: CTest labels genuinely absent (`test/CMakeLists.txt:223/354/407`); CPack
reads `${CMAKE_BINARY_DIR}/vcpkg_installed/` (`CPackConfig.cmake:26-27`); `DERIVED_VERSION_OVERRIDE`
honored (`GitVersionDerivation.cmake:48`); `.dockerignore:12-15` excludes `install`/`build`/`vcpkg_installed`;
`COPY --from=ci` at `Dockerfile:186`. The plan's factual anchors hold, but several **feasibility**
claims do not.

## MUST-FIX gaps

1. **[CRITICAL] Assembly-only Docker rests on an undocumented glibc/ABI coupling and silently breaks
   on the GitHub-hosted fallback.** The install tree bundles only the vcpkg `.so`s into
   `lib/aqualink-automate/` (`CPackConfig.cmake:99`) — **not** `libstdc++`/`libc`, resolved from the
   runtime base image. This works *today* only because the self-hosted Linux runner is Ubuntu 25.04 /
   GCC 15 — identical to the `ubuntu:25.04` runtime base. The instant `_build.yml` runs on
   `ubuntu-latest` (24.04: glibc 2.39, GCC 13 — the documented fallback), the artifact's
   `libstdc++`/`GLIBCXX`/`CXXABI` skew surfaces only at *container runtime, after publish*. **Fix:**
   build the Linux artifact *inside* an `ubuntu:25.04` `container:` (pin the ABI baseline regardless of
   host), or forbid the GitHub-hosted Linux fallback for the Docker-feeding artifact and assert
   runner-OS == runtime-base.
2. **[CRITICAL] The matter sidecar is not in the install tree** — `docker build --target runtime` still
   builds the `matter-builder` stage (`npm ci` + `tsc`) from source every time. "No recompile" is true
   only for the C++ half; the buildx context must still carry `matter-bridge/` + `docker-entrypoint.sh`.
3. **[HIGH] Scanners cannot consume prebuilt build trees.** A CMake build tree is not relocatable
   (`compile_commands.json`, `.ninja_deps`, response files embed absolute build-runner paths);
   CodeQL's DB is built by *observing the compiler run*. So `scan-linux`/`scan-windows` must compile
   in-job. The only shareable thing is the triplet-keyed **vcpkg binary cache** (config/path-independent).
   This collapses "scanners stop compiling" to "scanners stop *rebuilding vcpkg*."
4. **[HIGH] CodeQL + SonarCloud cannot share one compilation** — SonarCloud needs the
   `build-wrapper`-wrapped coverage build; CodeQL needs its own traced build; they are mutually
   exclusive wrappers on different configs. Folding into one job means **two compiles in one job**.
5. **[HIGH] "13→3" is materially overstated.** A single `release.yml` run still does 3× `_build.yml` +
   SonarCloud coverage + CodeQL + MSVC analysis + matter TypeScript ≈ **7 compiles**. The defensible
   win is killing duplicate **vcpkg** builds (the expensive part) via one shared cache + stopping the
   e2e/docker app-recompiles.

## SHOULD-FIX

6. **[MEDIUM]** `installtree-<plat>` upload/download (hundreds of MB of vcpkg `.so`s + assets) is added
   to the **critical path** of every PR (e2e + docker `needs:` it) — net latency may not improve as
   much as the compile-count drop suggests.
7. **[MEDIUM]** The bumped actions' Node24 runtime vs the self-hosted runner agent (`2.331.0`) is
   plausibly fine but **unverified** — add a runner-agent assertion or pin.
8. **[MEDIUM]** Docker smoke coverage shrinks: the cold from-source container build (which is exactly
   what would catch gap #1) only runs weekly under the plan. **Fix:** run it on every `main` push or
   every release tag as a blocking parallel check.
9. **[MEDIUM]** Per-run (not per-commit) reuse means the tagged merge commit is first built *at
   release*. The robust beta-killer fix is gating the develop→main **merge commit** with full CI
   (merge queue), which `version-check`-blocking does not do.

## MINOR / correct-as-claimed

10. CTest LABELS (Phase 0) — correct & low-risk; remember to add `-L` to the ctest *invocation*
    (presets have no filter, so labels alone change nothing in CI until then).
11. `.dockerignore` `!docker/context/` carve-out — sound but a footgun; smoke tests are the guardrail.
12. **OS/triplet-keying the vcpkg binary cache + dropping the per-job prefix is the highest-true-value
    change in the plan** and is correct — vcpkg binaries are config-independent.
13. Concurrency guard excluding main/develop + the orphan-prerelease-tag finalizer are correct and
    low-risk. Deferring release-please/auto-tag (fights the `git describe` + cut-from-main model) is right.

## Overall verdict

**Directionally sound and worth doing — but the plan oversells "build once," and two load-bearing
pillars (assembly-only Docker, scanners reusing build trees) are wrong as written.** Ship the
vcpkg-cache-keying and label work (the genuine wins) first; fix gaps #1–#4 before any
artifact-promotion claims; keep a per-release cold Docker build as a blocking gate.
