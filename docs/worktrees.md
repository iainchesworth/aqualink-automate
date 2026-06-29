# Git worktrees and parallel development

*For contributors and maintainers who run more than one branch at a time on a single machine. This is the source of truth for **where worktrees live, how they are named, and how to build one cheaply** without duplicating the ~8 GB vcpkg tree. The branch model and PR rules these worktrees feed into are in [CONTRIBUTING.md](CONTRIBUTING.md); building from source is in [INSTALL.md](INSTALL.md).*

You do not need worktrees to contribute. A single checkout with `git switch` is enough for one change at a time. Reach for a worktree when you want **several branches checked out at once** — reviewing one branch while building another, keeping a long-running build untouched while you start the next task, or fanning work out across parallel agents.

## Contents

- [Why worktrees](#why-worktrees)
- [Naming and layout](#naming-and-layout)
- [Branch namespaces](#branch-namespaces)
- [Creating a worktree](#creating-a-worktree)
- [The reuse-vcpkg recipe](#the-reuse-vcpkg-recipe)
- [Building and testing in a worktree](#building-and-testing-in-a-worktree)
- [Cleaning up](#cleaning-up)
- [Parallel and multi-agent orchestration](#parallel-and-multi-agent-orchestration)
- [Quick reference](#quick-reference)

## Why worktrees

A worktree is a second working directory backed by the same `.git`. Each worktree has its **own** checked-out branch, build directory, and uncommitted changes, but shares history, remotes, and the object store with the main checkout. That makes worktrees ideal for this project:

- One in-flight branch per worktree means a build in one never invalidates a build in another — no thrashing a single `build/` directory.
- Branches stay isolated, so parallel changes in disjoint source areas merge back with few or no conflicts (see [Parallel and multi-agent orchestration](#parallel-and-multi-agent-orchestration)).
- The expensive part of a build — the vcpkg dependency tree — can be **shared** from the main checkout rather than rebuilt per worktree (see [The reuse-vcpkg recipe](#the-reuse-vcpkg-recipe)).

## Naming and layout

Two rules keep a fleet of worktrees navigable:

1. **Worktrees live in a dedicated root *outside* the repository, on a disk with room to spare.** Do not nest a worktree inside the main checkout, and avoid a small or restricted volume — a second full debug build is several GB, and exhausting the disk surfaces as opaque linker/exit-code failures rather than a clear error. Pick one root directory and put every worktree under it.

2. **The worktree directory name matches the branch's short name.** Keep the directory name and the branch suffix aligned so it is always obvious which directory holds which branch.

| Placeholder | Meaning | Worked example (Windows) |
|-------------|---------|--------------------------|
| `<worktrees-root>` | The dedicated parent directory for all worktrees, outside the repo. | `C:\aa-wt` |
| `<name>` | A short task slug, reused for both the directory and the branch suffix. | `labels-cache` |
| `<main-tree>` | The path to your primary checkout (this repository). | `R:\aqualink-automate` |

So a worktree for a feature called `labels-cache` is the directory `<worktrees-root>/<name>` on branch `feat/<name>` — for example `C:\aa-wt\labels-cache` on `feat/labels-cache`.

> The Windows paths above are one maintainer's setup. On Linux or macOS a `~/aa-wt` root works exactly the same; substitute your own `<worktrees-root>` and `<main-tree>` throughout.

## Branch namespaces

Worktree branches follow the same naming as any other branch — see [CONTRIBUTING.md](CONTRIBUTING.md#branch-naming) for the authoritative rules. In short, `<type>/<name>` where `<type>` is an allowed commit type (`feat/`, `fix/`, `docs/`, `ci/`, `test/`, `refactor/`, `chore/`, `build/`, `perf/`). That naming is checked by a **Branch Name** CI job on every pull request, so name the branch correctly when you create the worktree — a misnamed branch fails CI.

One namespace is worktree-specific:

- **`claude/<name>`** — branches created by Claude Code's own managed worktrees, under the gitignored `.claude/worktrees/<name>` directory. These are **local scratch** branches, *not* part of the contribution convention. The assisted change is merged into `develop` **locally** (a direct merge then push — the normal flow here, and still allowed since `develop` requires no pull request); the `claude/*` branch itself is never pushed.

  Because branch naming is enforced on pull requests (the **Branch Name** required check), a `claude/*` head **cannot open a PR and cannot merge through GitHub**. If you want a pull request rather than a local merge, recreate the work on a `<type>/<name>` branch first.

## Creating a worktree

The bare command works on any platform:

```bash
git worktree add <worktrees-root>/<name> -b <type>/<name> develop
```

This creates the directory, a new branch off `develop`, and checks it out there. `develop` can stay checked out in the main tree at the same time — worktrees may share an upstream branch as their start point; they just cannot both *check out* the same branch.

A worktree created this way will, on first configure, try to populate its **own** `deps/vcpkg` submodule (~8 GB) and bootstrap vcpkg from scratch. That is wasteful when the main checkout already has a working vcpkg. The next section avoids it.

## The reuse-vcpkg recipe

The goal: a clean-source worktree that builds and tests, while **reusing the main checkout's already-bootstrapped vcpkg and its binary cache** — no multi-GB duplicate, no re-bootstrap.

Three things make this work:

1. **Skip the automatic submodule init.** The root `CMakeLists.txt` initialises `deps/vcpkg` before `project()` when `deps/vcpkg/.vcpkg-root` is missing (this is what makes a fresh clone and IDE "Open Folder" configure cleanly). In a worktree you do not want that, so drop an **empty** marker file to make the block skip:

   ```bash
   # from inside <worktrees-root>/<name>
   :> deps/vcpkg/.vcpkg-root          # create an empty file (any means works)
   ```

2. **Point the toolchain at the main tree's vcpkg.** Add a `CMakeUserPresets.json` *inside the worktree* (it is gitignored, so it stays local) with a preset that inherits the normal platform debug preset but overrides the toolchain file, the build directory, and clang-tidy:

   ```jsonc
   {
     "version": 9,
     "configurePresets": [
       {
         "name": "wt",
         "inherits": "config-windows-msvc-debug",
         "toolchainFile": "<main-tree>/deps/vcpkg/scripts/buildsystems/vcpkg.cmake",
         "binaryDir": "${sourceDir}/build/wt",
         "cacheVariables": { "ENABLE_CLANG_TIDY": "OFF" }
       }
     ],
     "buildPresets":   [ { "name": "wt", "configurePreset": "wt" } ],
     "testPresets":    [ { "name": "wt", "configurePreset": "wt", "output": { "outputOnFailure": true } } ]
   }
   ```

   On Linux/macOS, inherit `config-linux-gcc-debug` / `config-macos-llvm-debug` instead and use a forward-slash `<main-tree>` path. `ENABLE_CLANG_TIDY` is `ON` in the debug presets and runs per translation unit — turning it off is a large speed-up for a pure build/test verify.

3. **Build via the local preset.** Packages install to the worktree's own `binaryDir/vcpkg_installed` from the shared binary cache — nothing leaks back into the main tree's vcpkg root.

```bash
cmake --preset wt
cmake --build --preset wt
ctest --preset wt
```

**The one shared resource is the vcpkg `.vcpkg-root` lock.** Because the worktree points at the main tree's vcpkg, a configure can briefly block behind another build that is also touching vcpkg. This is fine for a quiet machine; if you routinely run many concurrent builds, give the worktree its **own** vcpkg instead — `git -C <worktrees-root>/<name> submodule update --init deps/vcpkg` — and drop the `toolchainFile` override. You trade ~8 GB of disk for zero lock contention.

> **Do not** combine "reuse the main vcpkg" with letting vcpkg *install* into a shared root (for example by junctioning `deps/vcpkg` to the main tree and still running install). That stalls every build on the filesystem lock and leaks writes across worktrees. Either reuse read-only via the toolchain override above, or give the worktree its own submodule — not a hybrid.

## Building and testing in a worktree

Everything in [INSTALL.md](INSTALL.md) applies unchanged inside a worktree; the only differences are the preset name (`wt` from your local `CMakeUserPresets.json`) and the build directory. A first configure that reuses the binary cache takes seconds rather than the multi-minute cold build.

On a disk-constrained machine, drop the build parallelism (`cmake --build --preset wt --parallel 2`) — a full-`-j` debug build of a second tree can exhaust a small volume, and out-of-disk link failures do not always print a clear compiler error.

## Cleaning up

Remove a worktree with git, not a raw directory delete — git has to drop the administrative link too:

```bash
git worktree remove --force <worktrees-root>/<name>
```

Use `--force` because the worktree has a populated build directory. If the worktree was given its **own** vcpkg submodule, pass `--force` **twice** (`git worktree remove --force --force …`) — a single `--force` silently no-ops when a populated submodule is present.

After removing worktrees, reclaim vcpkg scratch space in the main tree by deleting `deps/vcpkg/{buildtrees,packages,downloads}`; these are regenerated on demand and are safe to remove.

## Parallel and multi-agent orchestration

The same mechanics scale to several branches — or several automated agents — worked simultaneously, each in its own worktree off a common base. The approach is **compartmentalisation**: give every stream a disjoint slice of the source tree so they never touch the same lines.

- **One worktree, one branch, one stream of work**, all branched from the same base (`develop`). Disjoint source areas plus git's line-level auto-merge of additive `CMakeLists.txt` changes mean merges back to `develop` typically hit zero conflicts.
- **Merge order matters.** Land localised, additive features first; merge a broad restructuring or refactor **last** so it absorbs the others rather than forcing every other stream to rebase onto it.
- **Mind shared resources.** All worktrees share one vcpkg lock and one disk. Many simultaneous cold configures contend on the lock; many full builds contend on disk and CPU. Prefer the [reuse-vcpkg recipe](#the-reuse-vcpkg-recipe) during quiet windows, give worktrees their own vcpkg when you expect heavy concurrency, and lower `--parallel` when disk is tight.
- **Commit with an explicit pathspec.** When worktrees (or parallel agents) share an index or operate in the same tree, a bare `git commit` can sweep in another stream's staged changes. Commit your paths explicitly (`git commit -- <paths>`) and `git add` any brand-new files first — a pathspec commit does not stage untracked files.

## Quick reference

| Step | Command |
|------|---------|
| Create a worktree off `develop` | `git worktree add <worktrees-root>/<name> -b <type>/<name> develop` |
| Skip auto vcpkg init (reuse recipe) | `:> <worktrees-root>/<name>/deps/vcpkg/.vcpkg-root` |
| Configure / build / test (local preset) | `cmake --preset wt && cmake --build --preset wt && ctest --preset wt` |
| Give the worktree its own vcpkg instead | `git -C <worktrees-root>/<name> submodule update --init deps/vcpkg` |
| List worktrees | `git worktree list` |
| Remove a worktree | `git worktree remove --force <worktrees-root>/<name>` (twice-`--force` if it has its own vcpkg submodule) |
