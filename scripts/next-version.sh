#!/usr/bin/env bash
#
# next-version.sh — suggest the next release tag from Conventional Commits.
#
# PROPOSE-ONLY: this prints a recommendation and never creates or pushes a tag.
# A human still decides when a release line advances and pushes the tag (see
# docs/releasing.md). The git tag remains the single source of truth — nothing
# here is written to a manifest, so there is no dual-source-of-truth to drift.
#
# It classifies the Conventional Commits since the highest existing v* tag and
# maps them to a SemVer bump. Pre-1.0 (0.x) policy:
#
#   breaking change (`type!:` or `BREAKING CHANGE:` footer)  -> MINOR
#   feat                                                     -> MINOR
#   fix, perf                                                -> PATCH
#   docs, ci, chore, refactor, test, style, build, revert    -> not release-worthy
#
# (1.0.0 is reserved for the first stable; breaking changes bump the minor while
# the project is 0.x, matching how 0.2 -> 0.3 has moved.)
#
# Usage:
#   scripts/next-version.sh              # human-readable report to stdout
#   scripts/next-version.sh --markdown   # GitHub-flavoured markdown (PR comment / job summary)
#   scripts/next-version.sh --tag        # print ONLY the recommended tag, nothing else
#
# Exit status: 0 always (0 even when there is nothing release-worthy — the report
# says so). Non-zero only on a usage/git error.

set -euo pipefail

MODE="report"
case "${1:-}" in
	--markdown) MODE="markdown" ;;
	--tag) MODE="tag" ;;
	--help|-h)
		sed -n '2,40p' "$0" | sed 's/^# \{0,1\}//'
		exit 0
		;;
	"") ;;
	*) echo "unknown argument: $1 (try --help)" >&2; exit 2 ;;
esac

if ! git rev-parse --git-dir >/dev/null 2>&1; then
	echo "not a git repository" >&2
	exit 2
fi

# Anchor on the highest existing v* tag (version-sorted), NOT `git describe`:
# release tags live on main merge commits and are often not reachable from a
# develop checkout, so describe would miss them. The highest tag + `LAST..HEAD`
# yields exactly the commits being published, in both a develop checkout and a
# develop->main PR merge ref.
LAST="$(git tag --list 'v[0-9]*' | sort -V | tail -n1 || true)"

CHANNEL="beta"   # default prerelease channel when starting fresh
if [[ -z "$LAST" ]]; then
	# No tags yet: refuse to invent a base; suggest the conventional first prerelease.
	REC="v0.1.0-${CHANNEL}.1"
	LEVEL="initial"
	RANGE_DESC="(no existing v* tag)"
	FEATS=(); FIXES=(); BREAKS=()
else
	# Parse LAST into base + optional prerelease (channel.N).
	BASE="${LAST#v}"
	PRERELEASE=""
	if [[ "$BASE" == *-* ]]; then
		PRERELEASE="${BASE#*-}"   # e.g. beta.2
		BASE="${BASE%%-*}"        # e.g. 0.3.0
	fi
	IFS='.' read -r MAJOR MINOR PATCH <<<"$BASE"
	if [[ -n "$PRERELEASE" ]]; then
		CHANNEL="${PRERELEASE%%.*}"
		COUNTER="${PRERELEASE##*.}"
	fi
	RANGE_DESC="$LAST..HEAD"

	# Classify the commits being published.
	mapfile -t SUBJECTS < <(git log --no-merges --format='%s' "${LAST}..HEAD" 2>/dev/null || true)
	# Bodies (for BREAKING CHANGE footers), one record per commit, NUL-delimited.
	BREAKING=0
	FEATS=(); FIXES=(); BREAKS=()
	while IFS= read -r -d '' body; do
		[[ "$body" == *"BREAKING CHANGE"* || "$body" == *"BREAKING-CHANGE"* ]] && BREAKING=1
	done < <(git log --no-merges --format='%b%x00' "${LAST}..HEAD" 2>/dev/null || true)

	for s in "${SUBJECTS[@]}"; do
		if [[ "$s" =~ ^[a-z]+(\([^\)]*\))?!: ]]; then
			BREAKS+=("$s"); BREAKING=1
		elif [[ "$s" =~ ^feat(\([^\)]*\))?: ]]; then
			FEATS+=("$s")
		elif [[ "$s" =~ ^(fix|perf)(\([^\)]*\))?: ]]; then
			FIXES+=("$s")
		fi
	done

	# Map to a 0.x SemVer level.
	if [[ "$BREAKING" -eq 1 || ${#FEATS[@]} -gt 0 ]]; then
		LEVEL="minor"
	elif [[ ${#FIXES[@]} -gt 0 ]]; then
		LEVEL="patch"
	else
		LEVEL="none"
	fi

	# Candidate bases.
	NEXT_MINOR="${MAJOR}.$((MINOR + 1)).0"
	NEXT_PATCH="${MAJOR}.${MINOR}.$((PATCH + 1))"

	if [[ -n "$PRERELEASE" ]]; then
		CONTINUE="v${MAJOR}.${MINOR}.${PATCH}-${CHANNEL}.$((COUNTER + 1))"
		case "$LEVEL" in
			minor) REC="v${NEXT_MINOR}-${CHANNEL}.1"; ALT="$CONTINUE" ;;
			patch) REC="$CONTINUE"; ALT="" ;;
			none)  REC=""; ALT="$CONTINUE" ;;
		esac
	else
		# LAST was a stable release: the next dev iteration starts a new prerelease line.
		case "$LEVEL" in
			minor) REC="v${NEXT_MINOR}-${CHANNEL}.1"; ALT="v${NEXT_MINOR}" ;;
			patch) REC="v${NEXT_PATCH}-${CHANNEL}.1"; ALT="v${NEXT_PATCH}" ;;
			none)  REC=""; ALT="" ;;
		esac
	fi
fi

list() { if [[ ${#@} -eq 0 ]]; then echo "  (none)"; else printf '  - %s\n' "$@"; fi; }

if [[ "$MODE" == "tag" ]]; then
	echo "${REC:-}"
	exit 0
fi

if [[ "$MODE" == "markdown" ]]; then
	echo "<!-- next-version-suggestion -->"
	echo "### Suggested next release tag"
	echo
	if [[ "${LEVEL:-}" == "initial" ]]; then
		echo "No \`v*\` tag exists yet — suggested first prerelease: \`${REC}\`."
		exit 0
	fi
	echo "Comparing \`${RANGE_DESC}\` (${#FEATS[@]} feat, ${#FIXES[@]} fix/perf, ${#BREAKS[@]} breaking marked by \`!\`):"
	echo
	# Wrap each cell in backticks when set, else an em-dash / explanatory note.
	REC_MD="${REC:+\`$REC\`}"; REC_MD="${REC_MD:-_none — no release-worthy changes_}"
	ALT_MD="${ALT:+\`$ALT\`}"; ALT_MD="${ALT_MD:-—}"
	echo "| SemVer impact | Recommended tag | Alternative |"
	echo "|---|---|---|"
	echo "| **${LEVEL}** | ${REC_MD} | ${ALT_MD} |"
	echo
	if [[ "$LEVEL" == "minor" && -n "${PRERELEASE:-}" ]]; then
		echo "> The recommended tag opens a **new minor line** because a \`feat\`/breaking change is being published. If these changes belong to the *current* \`${BASE}\` target instead, continue the line with the alternative. **This is the human decision the policy reserves** — pick the tag and push it; nothing is auto-tagged."
	elif [[ "$LEVEL" == "none" ]]; then
		echo "> Only non-release-worthy commits (docs/ci/chore/etc.) since \`${LAST}\`. No new tag needed yet."
	fi
	exit 0
fi

# Default: human-readable report.
echo "Last tag:        ${LAST:-<none>}"
echo "Range examined:  ${RANGE_DESC}"
echo "SemVer impact:   ${LEVEL}"
echo "Recommended tag: ${REC:-<no release-worthy changes>}"
[[ -n "${ALT:-}" ]] && echo "Alternative:     ${ALT}"
echo
echo "Features (-> minor):"; list "${FEATS[@]}"
echo "Fixes/perf (-> patch):"; list "${FIXES[@]}"
echo "Breaking (-> minor while 0.x):"; list "${BREAKS[@]}"
