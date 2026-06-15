# ---------------------------------------------------------------------------
# stamp_sw_version.cmake
# ======================
# Rewrites the `const CACHE_VERSION = '...';` line in a (copied or installed)
# service-worker script so each build/package gets a unique service-worker cache
# name. That makes the activate-time purge drop the previous version's offline
# shell cache cleanly across upgrades. Online freshness already comes from the
# network-first fetch strategy in sw.js; this only governs the offline fallback.
#
# Invoked via:  cmake -D SW_JS=<path to sw.js> -D AQ_VERSION=<version> -P <this>
# Absent file (e.g. a doc-root without a service worker) is a quiet no-op.
# ---------------------------------------------------------------------------

if(NOT EXISTS "${SW_JS}")
    return()
endif()

if(NOT DEFINED AQ_VERSION OR AQ_VERSION STREQUAL "")
    set(AQ_VERSION "0")
endif()

file(READ "${SW_JS}" _sw_contents)
string(REGEX REPLACE
    "const CACHE_VERSION = '[^']*';"
    "const CACHE_VERSION = '${AQ_VERSION}';"
    _sw_contents "${_sw_contents}")
file(WRITE "${SW_JS}" "${_sw_contents}")
