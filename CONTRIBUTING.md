# Contributing to Aqualink Automate

*For contributors sending pull requests. Covers the branch model, the Conventional Commits format used in this repo, and what to run before you push. Building from source lives in [INSTALL.md](INSTALL.md).*

These are the guidelines we ask you to follow:

- [Git workflow](#git-workflow)
- [Submitting a pull request](#submitting-a-pull-request)
- [Merging](#merging)
- [Commit message format](#commit-message-format)
- [Allowed types and scopes](#allowed-types-and-scopes)
- [No attribution trailers](#no-attribution-trailers)
- [Release tagging](#release-tagging)
- [Building and testing before you push](#building-and-testing-before-you-push)

**Security:** do not report security vulnerabilities as normal issues or pull requests. Follow the private process in [SECURITY.md](SECURITY.md) instead.

There is no pull request or issue template in this repository — this guide is self-sufficient, so follow it directly.

## Git workflow

The repository has two core branches:

| Branch    | Purpose                                                    |
|-----------|-----------------------------------------------------------|
| `main`    | Production. Released, tagged code only.                    |
| `develop` | Integration branch. All non-hotfix work merges here first.|

The normal flow is `develop` -> `main`: features land on `develop`, and when a release is cut, `develop` is merged into `main` and a tag is created on `main` (see [Release tagging](#release-tagging)).

### Feature and bug branches

Create one branch per change, named `<type>/<branch-name>`, where `<type>` is one of the [allowed commit types](#allowed-types-and-scopes). Continuous integration triggers on the `feature/**` and `bug/**` namespaces, as well as on `main` and `develop` (see `.github/workflows/ci.yml`).

```bash
git switch develop
git switch -c feature/spaside-led-map      # a new feature
git switch -c bug/heater-setpoint-decode   # a bug fix
```

### Hotfix branches

A hotfix is an urgent fix that must reach production without waiting for the next `develop` release. Branch a hotfix off `main`:

```bash
git switch main
git switch -c fix/crash-on-empty-config
```

A hotfix merges into `main` (which is then tagged) and is also merged into `develop` so the fix is not lost on the next release.

## Submitting a pull request

For a **non-hotfix** change:

1. Branch off `develop` (for example `feature/my-new-feature`).
2. Make your change, **including appropriate test cases**.
3. Run the full test suite and confirm it passes — see [Building and testing before you push](#building-and-testing-before-you-push).
4. Commit using the [Conventional Commits format](#commit-message-format).
5. Open a pull request targeting `develop` and address any review feedback.

For a **hotfix** change:

1. Branch off `main`.
2. Open a pull request targeting `main`.
3. After it merges to `main`, also merge the fix into `develop`.

Every pull request must include test coverage for the change and must pass the full suite. CI runs the C++ build and tests, the Playwright UI end-to-end specs, the Matter bridge checks, and a Docker image verification on each pull request. See [docs/ci-cd.md](docs/ci-cd.md) for the catalogue of what runs and where, and [`.github/workflows/ci.yml`](.github/workflows/ci.yml) for the authoritative list of PR checks.

## Merging

The default is **Squash and Merge** into `develop`. A single squashed commit per feature or bug fix keeps history readable and makes it straightforward to revert one change in isolation.

Before merging, set the pull request title to a valid [Conventional Commit](#commit-message-format) — the squash commit uses the pull request title as its subject.

If a pull request bundles several distinct features or fixes, use your judgement:

- **Squash and Merge** collapses everything into one commit on `develop`.
- **Create a merge commit** preserves the individual commits from your branch.

Prefer Squash and Merge unless preserving the separate commits adds real value.

## Commit message format

This project uses [Conventional Commits](https://www.conventionalcommits.org/). Each commit message has this shape:

```
<type>(<scope>): <subject>

<body>

<footer>
```

- **type** — required. One of the [allowed types](#allowed-types-and-scopes).
- **scope** — optional. The area of the codebase affected, in parentheses. Multiple scopes are comma-separated.
- **subject** — required. A short, imperative, present-tense summary on the first line ("add", not "added" or "adds"). No trailing period; keep it under about 72 characters.
- **body** — optional. Explains the motivation and what changed, in present tense. Separate it from the subject with a blank line.
- **footer** — optional. References issues (for example `Closes #42`) and records breaking changes.

Real examples from this repository:

```text
fix(cicd): keep runner thin disks lean with discard + hourly fstrim
ci(codescanning): scan only new code; skip the develop->main duplicate
test(integration): expect median-smoothed hub salt in transitions replay
refactor(webui): give Schedules form inputs their own .sched-input class
feat(webui,jandy): tailored device cards, robust command dispatch, RSSA presence-gating fix
docs(spaside): mark the iAQ spa-switch writer IMPLEMENTED
```

A complete message with a body and footer:

```text
fix(chlorinator): smooth AquaRite salt/health flapping

The reported salt concentration flapped between adjacent samples because
each raw reading was published unfiltered. Apply median smoothing before
writing to the DataHub and surface cell warnings.

Closes #57
```

### Breaking changes

Mark a backward-incompatible change with a `!` before the colon, and describe it in a `BREAKING CHANGE:` footer:

```text
feat(api)!: rename expected_auxillary_count to expected_aux_count

BREAKING CHANGE: the /api/equipment response field expected_auxillary_count
is renamed to expected_aux_count. Update any client that reads it.
```

### Reverts

To revert a previous commit, use the `revert` type and name the reverted subject and hash in the body:

```text
revert: feat(webui,jandy): tailored device cards and robust command dispatch

This reverts commit a1b2c3d. The command-dispatch change regressed the
OneTouch home page; reverting until the RSSA gating is fixed.
```

## Allowed types and scopes

Use one of these types. Put the area of the code in the **scope**, not the type.

| Type       | Use for                                                        |
|------------|----------------------------------------------------------------|
| `feat`     | A new feature or capability.                                    |
| `fix`      | A bug fix.                                                      |
| `docs`     | Documentation only.                                             |
| `ci`       | CI/CD workflows, GitHub Actions, runner configuration.         |
| `test`     | Adding or correcting tests.                                    |
| `refactor` | A code change that neither fixes a bug nor adds a feature.      |
| `chore`    | Maintenance, tooling, dependencies, housekeeping.              |
| `build`    | Build system, CMake, vcpkg, packaging.                         |
| `perf`     | A change that improves performance.                            |

**Scopes** name the affected area and are free-form, but stay consistent with what history already uses. Common scopes include `jandy`, `pentair`, `webui`, `chlorinator`, `cicd`, `matter`, `spaside`, `iaq`, `http`, and `integration`. Combine scopes with commas when a change spans areas, for example `feat(webui,jandy): ...`.

**Important:** older commits use ad-hoc prefixes such as `jandy:`, `release:`, `spaside:`, `epump:`, `core:`, and `docker:` as the type. These are **not** valid Conventional Commit types — they are scopes. Use a type from the table above and move the area name into the scope: write `fix(jandy): ...`, not `jandy: ...`.

## No attribution trailers

Do not add attribution or co-authorship trailers to commit messages, pull request descriptions, or any generated text. This includes `Co-Authored-By:` lines and any equivalent "co-authored" or attribution statement. This is a hard project rule — keep commits and pull request bodies free of attribution trailers.

## Release tagging

Releases are tagged on `main` using a plain semantic-version tag prefixed with `v` — for example `v1.0.0`, or a prerelease such as `v1.0.0-beta.1`. Pushing a `v*` tag triggers the release pipeline (`.github/workflows/release.yml`), which builds, packages, publishes the Docker image, and creates the GitHub release. The pipeline rejects any tag that is not a valid `v<MAJOR>.<MINOR>.<PATCH>[-(alpha|beta|rc).<N>]` and requires the tagged commit to be contained in `main`.

Do not use the old `release-YYYYMMDD-vX.Y.Z` naming — it is not what the pipeline matches.

For the full version scheme, prerelease labels, and the step-by-step release procedure, see [docs/releasing.md](docs/releasing.md). Most contributors do not tag releases.

## Building and testing before you push

Build the project and run the full test suite before opening a pull request. The repository ships convenience build scripts and CMake presets; the full setup, prerequisites, and per-platform notes are in [INSTALL.md](INSTALL.md).

Configure, build, and test with the matching presets (replace `config-`/`build-`/`test-` to pick the platform preset listed in [INSTALL.md](INSTALL.md)):

```bash
cmake --preset config-linux-gcc
cmake --build --preset build-linux-gcc
ctest --preset test-linux-gcc
```

On Windows the equivalent presets are `config-windows-msvc-debug`, `build-windows-msvc-debug`, and `test-windows-msvc-debug`.

All tests must pass and your change must include tests covering it. A bug fix must add a regression test that fails before the fix and passes after it.
