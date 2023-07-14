# Contributing to Aqualink Automate

Here are guidelines we would like you to follow are:

 - [Git Workflow](#git-workflow)
 - [Submission Guidelines](#submitting-a-pull-request-pr)
 - [Commit Message Guidelines](#git-commit-guidelines)

### Git Workflow

This repository has two core branches: **main** (production) and **develop**.

Normal workflow: `develop` -> `main` (tag latest commit)

#### Feature Branches

New features should have its own branches with name format: `[type]/[branch-name]` where type follows [commit type](#type).

#### Release Tag

Release tag should be made on the **latest** in master after merging `develop` to `main`.

Naming convention with semantic versioning: release-YYYYMMDD-vX.Y.Z

#### Hotfix branch

Branch off of main and upon completion of fix, merge into `main` (tag hotfix) and `develop` as a **merge commit**. 

### Submitting a Pull Request (PR)

Before you submit your Pull Request (PR) for a **non-hotfix** task, consider the following guidelines:

1. Branch off of `develop` as `feature/my-new-feature`
2. Add your update, **including appropriate test cases.**
3. Run the full test suite and ensure all tests pass.
4. Commit your changes using a descriptive commit message that follows our [commit message conventions](#commit).
5. Open PR to `develop` and make any requested changes (if any)

#### Merging your pull request (non-hotfix)

If you're merging a PR that addresses one single feature or bugfix:

1. Before merging your PR, please change the name as the name must follow [commit message conventions](#commit)
2. **Squash and Merge** your changes into `develop`, this will make it easier to revert specific features, bugfixes, or issues if the need arises.

If you're merging a PR that addresses multiple features and/or bugfixes:

1. Use your best judgement whether to `Squash and Merge` or `Create a merge commit`
  * `Squash and merge` will merge your changes into `develop` as a single commit
  * `Create a merge commit` will merge your changes into `develop` including all the commits made in your main branch. 

#### Merging your hotfix pull requests

When submitting your PR for a **hotfix** tasks, please follow the following:
1. Branch off the `main` branch
2. Open PR to `main`
3. After hotfix is merged into main, merge the hotfix changes to develop as a `merge commit` type.

### Git Commit Guidelines

We have guidelines on how git commit messages can be formatted.  This leads to **more readable messages** that are easy to follow when looking through the **project history**.

#### Commit Message Format

TBC

#### Revert

TBC

### Type

Must be one of the following:

TBC

#### Scope

TBC

#### Subject

TBC

#### Body

TBC

#### Footer

TBC