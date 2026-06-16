#!/usr/bin/env bash
#
# Container entrypoint: launch the aqualink-automate app (authoritative) plus the
# Matter bridge sidecar (supervised, restartable, NON-fatal), forward termination
# signals to both, and bring the container down ONLY when the app exits. tini is
# PID 1 (see the Dockerfile ENTRYPOINT) and reaps zombies + forwards signals to this
# script. When started as root it first applies the configurable PUID/PGID (default
# 10000) and drops privileges to that user via gosu.
#
# Process model:
#   - The APP is the primary service: when it exits, the container exits with the
#     app's status (and the orchestrator's restart policy reacts).
#   - The Matter SIDECAR is secondary: if it crashes it is restarted with capped
#     backoff, and a sidecar failure never takes the pool controller (the app)
#     offline -- a Matter/mDNS hiccup must not knock out pool automation.
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

BACKOFF_MAX=15   # cap the sidecar restart backoff (seconds)

app_pid=""
sidecar_pid=""
matter_enabled=0
shutting_down=0
status=0

start_sidecar() {
    node "$SIDECAR" &
    sidecar_pid=$!
    echo "[entrypoint] Matter bridge sidecar started (pid $sidecar_pid)"
}

# SIGTERM/SIGINT: forward to both children and stop supervising. The app's own
# shutdown then drives the exit below.
terminate() {
    shutting_down=1
    [ -n "$sidecar_pid" ] && kill -TERM "$sidecar_pid" 2>/dev/null || true
    [ -n "$app_pid" ]     && kill -TERM "$app_pid" 2>/dev/null || true
}
trap terminate TERM INT

# Interruptible sleep: an incoming signal runs the trap and unblocks the wait
# immediately (a foreground `sleep` would keep blocking for its full duration).
nap() {
    sleep "$1" &
    wait $! 2>/dev/null || true
}

echo "[entrypoint] starting aqualink-automate"
"$APP" "$@" &
app_pid=$!

if [ "${MATTER_ENABLED:-true}" = "false" ]; then
    echo "[entrypoint] Matter disabled (MATTER_ENABLED=false); not starting the sidecar"
elif ! command -v node >/dev/null 2>&1 || [ ! -f "$SIDECAR" ]; then
    echo "[entrypoint] Matter enabled but the sidecar/node runtime is missing; skipping (the app runs without it)"
else
    matter_enabled=1
    start_sidecar
fi

# Supervise: wait for whichever child exits next (-p records which). The app exiting
# is terminal; the sidecar exiting is recoverable (restart with capped backoff), so a
# Matter failure never takes the container (the pool controller) down.
backoff=1
while :; do
    # `wait -p` UNSETS the variable when no job is reaped (e.g. a signal interrupts
    # the wait), so normalise it afterwards to stay safe under `set -u`.
    if wait -n -p reaped; then rc=0; else rc=$?; fi
    reaped="${reaped:-}"

    # A signal interrupted the wait (nothing actually reaped). If the app is gone,
    # fall through to shutdown; otherwise keep supervising.
    if [ -z "$reaped" ]; then
        kill -0 "$app_pid" 2>/dev/null || break
        continue
    fi

    if [ "$reaped" = "$app_pid" ]; then
        status=$rc
        app_pid=""
        echo "[entrypoint] aqualink-automate exited (status $status); stopping the container"
        break
    fi

    if [ "$matter_enabled" -eq 1 ] && [ "$reaped" = "$sidecar_pid" ]; then
        sidecar_pid=""
        [ "$shutting_down" -eq 0 ] || break
        echo "[entrypoint] Matter sidecar exited (status $rc); restarting in ${backoff}s (the app keeps running)"
        nap "$backoff"
        [ "$shutting_down" -eq 0 ] || break
        # Only restart while the app is still alive. If the app died during the
        # backoff, do NOT break here: loop back so `wait -n` reaps its real exit
        # status (breaking here would lose it and exit the container with 0).
        if kill -0 "$app_pid" 2>/dev/null; then
            backoff=$(( backoff < BACKOFF_MAX ? backoff * 2 : BACKOFF_MAX ))
            start_sidecar
        fi
    fi
done

echo "[entrypoint] shutting down; stopping any remaining supervised process"
terminate
wait 2>/dev/null || true
exit "$status"
