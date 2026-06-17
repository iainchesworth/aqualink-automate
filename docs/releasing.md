# Releasing Aqualink Automate

*For maintainers cutting a tagged release. Building from source lives in [INSTALL.md](../INSTALL.md); the workflow internals live in [docs/ci-cd.md](ci-cd.md) (the original redesign plan is in [docs/cicd-redesign.md](cicd-redesign.md)).*

## Version scheme

This project follows [Semantic Versioning 2.0.0](https://semver.org/):

```
<MAJOR>.<MINOR>.<PATCH>[-<prerelease>]
```

- **MAJOR** — incompatible API/protocol changes
- **MINOR** — new features, backward-compatible
- **PATCH** — bug fixes, backward-compatible

### Prerelease labels

Prerelease versions use the format `<label>.<N>`:

| Label    | Purpose                              | Example        |
|----------|--------------------------------------|----------------|
| `alpha`  | Early development, unstable          | `1.0.0-alpha.1`|
| `beta`   | Feature-complete, testing in progress| `1.0.0-beta.2` |
| `rc`     | Release candidate, final validation  | `1.0.0-rc.1`   |

The release workflow validates every tag against `v<M>.<M>.<P>[-(alpha|beta|rc).<N>]`. Keep tag naming aligned with the conventions in [CONTRIBUTING.md](../CONTRIBUTING.md).

## Version source of truth

- **CI builds**: Version is derived from git tags at configure time via `cmake/GitVersionDerivation.cmake`. The tag `v1.0.0` produces version `1.0.0`; `v1.0.0-beta.2` produces version `1.0.0` with prerelease `beta.2`.
- **Local builds without tags**: Falls back to `0.0.0-dev`, making it obvious this is not a release build.
- **CMake `project()`**: Receives only the `M.M.P` portion (CMake doesn't support prerelease in `VERSION`). The full version including prerelease is available via `PROJECT_VERSION_FULL`.

## Repository configuration

### Docker Hub credentials (optional but recommended)

The `runtime`/`ci` Docker images build `FROM ubuntu:25.04` and `FROM node:22-bookworm-slim`, which are pulled from Docker Hub. Anonymous pulls share the runner's egress IP and are subject to Docker Hub's anonymous per-IP rate limit — a recurring CI reliability risk, especially on **self-hosted runners** (one shared IP across many jobs) and during Docker Hub CDN hiccups.

Configure these **optional** repository secrets to authenticate base-image pulls and get a higher rate limit:

| Secret              | Value                                                                 |
|---------------------|-----------------------------------------------------------------------|
| `DOCKERHUB_USERNAME`| A Docker Hub account username                                         |
| `DOCKERHUB_TOKEN`   | A Docker Hub **access token** (Account Settings → Personal access tokens; `Public Repo Read-only` scope is sufficient) |

Both the release `docker-publish` job and the CI `docker-verify` job log in to Docker Hub **only when both secrets are present** (`if: env.DOCKERHUB_USERNAME != ''`). If they are unset — forks, or before they are configured — the workflows fall back to anonymous pulls and still work. Nothing else is required.

The Docker build steps are additionally wrapped in a **bounded retry** (`nick-fields/retry`, 3 attempts), so a single transient base-image blob timeout retries instead of failing the whole release; the retry reuses the BuildKit layer cache, so it only re-fetches the layer that timed out.

> **Self-hosted runners:** for heavily-used self-hosted runners you can additionally configure a [pull-through registry mirror](https://docs.docker.com/docker-hub/image-library/mirror/) on the runner host (a local registry caching Docker Hub, referenced via `registry-mirrors` in `/etc/docker/daemon.json` and the BuildKit builder config). This is host/daemon configuration — not part of these workflow files — and complements the authentication above.

## Pre-release checklist

Before creating a release:

1. CI is green on `main` (all platforms pass).
2. All intended changes are merged to `main`. Real releases must be **cut from main**: the release commit has to be an ancestor of `origin/main`. The `resolve-version` job enforces this with `git merge-base --is-ancestor` and hard-fails any non-dry-run release whose commit is not contained in `main` (merge `develop` → `main` first, then tag `main`). Dry runs are exempt, so you can validate the pipeline from `develop`.
3. Example configs in `examples/` are current.
4. Release notes are reviewed. The `github-release` job seeds the release body with `gh release create --generate-notes` (a flat list of merged PRs / commits since the previous tag) — treat that as a **first draft only**. Make sure [CHANGELOG.md](../CHANGELOG.md) and the PR titles read well (they feed the draft), then **curate the published body to the project's established pattern** — this is a required step, described under [Post-release](#post-release).
5. Run a local build to verify version output: `./aqualink-automate --version`.

### Run the test suites by label

CTest tags each suite with a label (`unit`, `integration`, `perf`), so you can gate which tests run before tagging:

```bash
ctest --preset test-windows-msvc-debug -L unit          # fast suite — local / PR gate
ctest --preset test-windows-msvc-debug -L integration   # slower fixture-replay suite
ctest --preset test-windows-msvc-debug -L perf          # Google Benchmark performance tests
```

CTest `--preset` only accepts a **test** preset, so use the `test-*` preset that matches the configure preset you built with — swap the `config-` prefix for `test-` (see [INSTALL.md](../INSTALL.md)). Omit `-L` to run every registered suite.

## Option A: tag-based release

This is the standard release method. Pushing a `v*` tag triggers the release workflow automatically.

```bash
# Ensure you're on the latest main
git checkout main
git pull origin main

# Create and push the tag
git tag v1.0.0
git push origin v1.0.0
```

For a prerelease:

```bash
git tag v1.0.0-beta.1
git push origin v1.0.0-beta.1
```

Monitor the release at: **Actions > Release** in GitHub.

## Option B: manual dispatch

Use the GitHub Actions UI to trigger a release manually. This is useful for creating releases from a specific commit (still constrained to commits already on `main`) or for testing.

1. Go to **Actions > Release > Run workflow**.
2. Fill in the inputs:
   - **version**: e.g. `v1.0.0` or `v1.0.0-rc.1`.
   - **prerelease**: check if this is a prerelease (auto-detected from tag suffix).
   - **dry_run**: check to build packages without creating a GitHub Release.

For a manual dispatch the tag does **not** exist yet. The `resolve-version` job fails fast if the tag is already present (so you don't build every package only to collide at the end), and the tag is created and pushed by the `github-release` job **at the end of a successful run** — after the build, Docker publish, and package jobs all pass. A failed dispatch therefore leaves no tag behind, so you can fix the problem and re-run the same version.

## Dry run

A dry run builds packages on all platforms without creating a GitHub Release or publishing Docker images. Use this to verify packaging before a real release. Dry runs are exempt from the "cut from main" guard, so you can run one from `develop`.

1. Go to **Actions > Release > Run workflow**.
2. Enter the version tag.
3. Check **dry_run**.
4. Download build artifacts from the workflow run to inspect packages.

## Post-release

After a release is published:

1. **Curate the release notes to the established pattern (required).** The auto-generated body (`--generate-notes`) is only a starting point; rewrite it so the notes read as a coherent, user-facing changelog consistent with the previous releases. Use a prior release as the template (`gh release view v0.2.0-beta.1 --json body --jq .body`) and mirror the matching `## [x.y.z]` section of [CHANGELOG.md](../CHANGELOG.md). The structure, **in this order**:
   1. `## What's Changed since v<prev>` heading (for the very first release: `## What's Changed`).
   2. A one-line **summary** of the release.
   3. The changes as `###` subsections — grouped by subsystem (e.g. *Trends*, *Web UI*) or as *Added* / *Changed* / *Fixed* — with **bold lead-in** bullets in user-facing terms (not raw commit subjects). A short release may use a flat **bold-lead-in** bullet list instead of subsections.
   4. An `## Artifacts` section: the "Self-contained, SHA-512-summed" table (Linux / Windows / macOS / Docker, including the `ghcr.io/<owner>/aqualink-automate:<version>` image tag), then the `aqualink-automate --help` / [INSTALL.md](../INSTALL.md) pointer line.
   5. The `**Full Changelog**: …/compare/v<prev>...v<this>` link (keep the one from the generated notes; omit for the first release).
   6. For prereleases, a trailing `> **Pre-release.**` caveat blockquote (e.g. the unverified-Pentair-decoding caveat).

   Apply the curated body with:

   ```bash
   gh release edit vX.Y.Z[-beta.N] --notes-file notes.md
   ```

2. Verify the GitHub Release page has all expected artifacts.
3. Verify Docker images are published to GHCR.
4. Verify the curated notes render correctly and read well; tweak the body if anything reads poorly.

## Release artifacts

Each release includes:

| Platform | Packages                     |
|----------|------------------------------|
| Linux    | `.tgz`, `.deb`, `.rpm`       |
| Windows  | `.zip`, `.exe` (NSIS)        |
| macOS    | `.tgz`, `.dmg`               |

Additionally:
- **Checksums**: `.sha512` files for every package (`CPACK_PACKAGE_CHECKSUM SHA512`).
- **Bundled runtime libraries**: the vcpkg-provided shared libraries ship inside each package (private lib dir with RPATH/loader-path), so the binary runs without a separate dependency install.
- **Example configs**: the `examples/*.conf` files are bundled in each package.
- **Docker image**: Published to `ghcr.io/<owner>/aqualink-automate`. A stable release also moves the `latest` tag; a prerelease moves the `edge` tag instead. Both carry the exact `<version>` tag (e.g. `0.3.0-beta.2`); prereleases do not move the `<major>.<minor>` tag.

These packages are produced by CPack via the matching `pack-*` presets. `pack-*` presets exist only for the **Release** configure presets (those with no `-debug`/`-coverage` suffix), so swap the `config-` prefix for `pack-` only on a Release preset — for example `config-linux-gcc` → `pack-linux-gcc`. See [INSTALL.md](../INSTALL.md) for the local pack-* preset workflow.

## Troubleshooting

### Version shows `0.0.0-dev` in CI

The checkout step needs `fetch-depth: 0` so `git describe` can find tags. Verify the workflow has this setting.

### Version shows `0.0.0-dev` locally

No `v*` tag is reachable from HEAD. Create a tag or fetch tags:

```bash
git fetch --tags
git describe --tags --match "v*" --always HEAD
```

### `git describe` returns unexpected results

Ensure tags follow the `v<M>.<M>.<P>` format exactly. Lightweight and annotated tags both work.

### Package filename doesn't include prerelease

The prerelease suffix is only added when `PROJECT_VERSION_PRERELEASE` is non-empty. Verify the tag includes a prerelease suffix (e.g. `v1.0.0-beta.1`).

### Release fails because the commit is not on main

The `resolve-version` job rejects any non-dry-run release whose commit is not an ancestor of `origin/main` (`git merge-base --is-ancestor`). Merge your changes to `main` first (`develop` → `main`), then tag or dispatch from a commit on `main`. To validate the pipeline from another branch without merging, use a dry run.

### Docker build fails pulling a base image (rate limit or CDN timeout)

Symptoms: `docker-publish` (release) or `docker-verify` (CI) fails while pulling `ubuntu:25.04` or `node:22-bookworm-slim`, with errors like `toomanyrequests: ... rate limit` or `dial tcp ...(production.cloudfront.docker.com): i/o timeout`.

- The build steps already retry up to 3 times, so a one-off CDN timeout should self-heal — check whether the run eventually succeeded on a later attempt.
- If you see rate-limit (`toomanyrequests`) errors, configure the `DOCKERHUB_USERNAME` / `DOCKERHUB_TOKEN` secrets (see [Docker Hub credentials](#docker-hub-credentials-optional-but-recommended)) so pulls are authenticated.
- On self-hosted runners, consider a pull-through registry mirror (same section) to remove the dependency on Docker Hub's CDN for repeat builds.

### Manual dispatch tag already exists

The `resolve-version` job rejects a dispatch whose tag already exists on the remote (rather than failing later at the push step). Delete the existing tag first, then re-run:

```bash
git push origin :refs/tags/v1.0.0-beta.1
git tag -d v1.0.0-beta.1
```
