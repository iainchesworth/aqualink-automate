#!/usr/bin/env bash
#
# Container entrypoint: launch the Matter bridge sidecar (unless opted out) and the
# aqualink-automate app, supervise both, forward termination signals to both, and
# bring the container down if EITHER exits. tini is PID 1 (see the Dockerfile
# ENTRYPOINT) and reaps zombies + forwards signals to this script. When started as
# root it first applies the configurable PUID/PGID (default 10000) and drops
# privileges to that user via gosu.
#
# Opt out of Matter with MATTER_ENABLED=false (mirrors the app's --matter false).
# Everything after the script name is forwarded verbatim to the app (the image CMD).

set -euo pipefail

APP=/opt/aqualink-automate/bin/aqualink-automate
SIDECAR=/opt/matter-bridge/dist/index.js

# ── Privilege drop (configurable PUID/PGID) ─────────────────────────────────
# The image starts as root. Remap the bundled `aqualink` user to the requested
# PUID/PGID (defaults 10000), fix ownership of the writable paths, then re-exec
# this script as that unprivileged user via gosu. Set PUID=0 to stay root.
# If the container is started as non-root (e.g. via the compose `user:` directive)
# this branch is skipped and the caller is responsible for owning the bind mounts.
PUID="${PUID:-10000}"
PGID="${PGID:-10000}"

if [ "$(id -u)" = "0" ]; then
    if [ "$PUID" != "0" ]; then
        groupmod -o -g "$PGID" aqualink
        usermod  -o -u "$PUID" -g "$PGID" aqualink
    fi
    # /etc/aqualink is a read-only config mount, so it is intentionally NOT chowned
    # here; the config must be readable by PUID on the host instead.
    chown -R "$PUID:$PGID" /data /home/aqualink 2>/dev/null || true
    if [ "$PUID" != "0" ]; then
        echo "[entrypoint] dropping privileges to ${PUID}:${PGID}"
        exec gosu "$PUID:$PGID" "$0" "$@"
    fi
    echo "[entrypoint] PUID=0 requested; continuing as root"
fi

pids=""

terminate() {
    # Forward the signal to every child; ignore "no such process" races.
    [ -n "$pids" ] && kill -TERM $pids 2>/dev/null || true
}
trap terminate TERM INT

if [ "${MATTER_ENABLED:-true}" != "false" ]; then
    if command -v node >/dev/null 2>&1 && [ -f "$SIDECAR" ]; then
        echo "[entrypoint] starting Matter bridge sidecar"
        node "$SIDECAR" &
        pids="$pids $!"
    else
        echo "[entrypoint] Matter enabled but the sidecar/node runtime is missing; skipping"
    fi
else
    echo "[entrypoint] Matter disabled (MATTER_ENABLED=false); not starting the sidecar"
fi

echo "[entrypoint] starting aqualink-automate"
"$APP" "$@" &
pids="$pids $!"

# Exit as soon as ANY supervised process exits, so a crashed app or sidecar takes
# the container down (and the orchestrator's restart policy can react).
wait -n
status=$?

echo "[entrypoint] a supervised process exited (status $status); stopping the rest"
terminate
wait || true
exit "$status"
