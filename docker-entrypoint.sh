#!/usr/bin/env bash
#
# Container entrypoint: launch the Matter bridge sidecar (unless opted out) and the
# aqualink-automate app, supervise both, forward termination signals to both, and
# bring the container down if EITHER exits. tini is PID 1 (see the Dockerfile
# ENTRYPOINT) and reaps zombies + forwards signals to this script.
#
# Opt out of Matter with MATTER_ENABLED=false (mirrors the app's --matter false).
# Everything after the script name is forwarded verbatim to the app (the image CMD).

set -euo pipefail

APP=/opt/aqualink-automate/bin/aqualink-automate
SIDECAR=/opt/matter-bridge/dist/index.js

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
