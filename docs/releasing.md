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

### Manual dispatch tag already exists

If the tag already exists on the remote, the workflow will fail when trying to push it. Delete the existing tag first:

```bash
git push origin :refs/tags/v1.0.0-beta.1
git tag -d v1.0.0-beta.1
```
