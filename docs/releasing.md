# Releasing Aqualink Automate

## Version Scheme

This project follows [Semantic Versioning 2.0.0](https://semver.org/):

```
<MAJOR>.<MINOR>.<PATCH>[-<prerelease>]
```

- **MAJOR** — incompatible API/protocol changes
- **MINOR** — new features, backward-compatible
- **PATCH** — bug fixes, backward-compatible

### Prerelease Labels

Prerelease versions use the format `<label>.<N>`:

| Label    | Purpose                              | Example        |
|----------|--------------------------------------|----------------|
| `alpha`  | Early development, unstable          | `1.0.0-alpha.1`|
| `beta`   | Feature-complete, testing in progress| `1.0.0-beta.2` |
| `rc`     | Release candidate, final validation  | `1.0.0-rc.1`   |

## Version Source of Truth

- **CI builds**: Version is derived from git tags at configure time via `cmake/GitVersionDerivation.cmake`. The tag `v1.0.0` produces version `1.0.0`; `v1.0.0-beta.2` produces version `1.0.0` with prerelease `beta.2`.
- **Local builds without tags**: Falls back to `0.0.0-dev`, making it obvious this is not a release build.
- **CMake `project()`**: Receives only the `M.M.P` portion (CMake doesn't support prerelease in `VERSION`). The full version including prerelease is available via `PROJECT_VERSION_FULL`.

## Repository Configuration

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

## Pre-release Checklist

Before creating a release:

1. CI is green on `main` (all platforms pass)
2. All intended changes are merged to `main`
3. Example configs in `examples/` are current
4. Changelog / release notes are prepared
5. Run a local build to verify version output: `./aqualink-automate --version`

## Option A: Tag-Based Release

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

## Option B: Manual Dispatch

Use the GitHub Actions UI to trigger a release manually. This is useful for creating releases from specific commits or for testing.

1. Go to **Actions > Release > Run workflow**
2. Fill in the inputs:
   - **version**: e.g. `v1.0.0` or `v1.0.0-rc.1`
   - **prerelease**: check if this is a prerelease (auto-detected from tag suffix)
   - **dry_run**: check to build packages without creating a GitHub Release

The workflow will create and push the tag to the repository.

## Dry Run

A dry run builds packages on all platforms without creating a GitHub Release or publishing Docker images. Use this to verify packaging before a real release.

1. Go to **Actions > Release > Run workflow**
2. Enter the version tag
3. Check **dry_run**
4. Download build artifacts from the workflow run to inspect packages

## Post-Release

After a release is published:

1. Verify the GitHub Release page has all expected artifacts
2. Verify Docker images are published to GHCR
3. Verify the release notes are accurate

## Release Artifacts

Each release includes:

| Platform | Packages                     |
|----------|------------------------------|
| Linux    | `.tgz`, `.deb`, `.rpm`       |
| Windows  | `.zip`, `.exe` (NSIS)        |
| macOS    | `.tgz`, `.dmg`               |

Additionally:
- **Checksums**: `.sha512` files for every package
- **Example configs**: Bundled in the `examples/` directory within each package
- **Docker image**: Published to `ghcr.io/<owner>/aqualink-automate` (not for prereleases tagged `latest`)

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

### Docker build fails pulling a base image (rate limit or CDN timeout)

Symptoms: `docker-publish` (release) or `docker-verify` (CI) fails while pulling `ubuntu:25.04` or `node:22-bookworm-slim`, with errors like `toomanyrequests: ... rate limit` or `dial tcp ...(production.cloudfront.docker.com): i/o timeout`.

- The build steps already retry up to 3 times, so a one-off CDN timeout should self-heal — check whether the run eventually succeeded on a later attempt.
- If you see rate-limit (`toomanyrequests`) errors, configure the `DOCKERHUB_USERNAME` / `DOCKERHUB_TOKEN` secrets (see [Docker Hub credentials](#docker-hub-credentials-optional-but-recommended)) so pulls are authenticated.
- On self-hosted runners, consider a pull-through registry mirror (same section) to remove the dependency on Docker Hub's CDN for repeat builds.

### Manual dispatch tag already exists

If the tag already exists on the remote, the workflow will fail when trying to push it. Delete the existing tag first:

```bash
git push origin :refs/tags/v1.0.0-beta.1
git tag -d v1.0.0-beta.1
```
