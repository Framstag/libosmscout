#!/bin/sh
# mapgen-pass.sh - one regeneration pass, guarded against a concurrent pass.
#
# A pass assumes it is the only writer of the repository's private/ area. A
# scheduled occurrence and an externally triggered pass can otherwise overlap,
# so the pass runs under an exclusive lock inside the work area: a second
# invocation reports the collision and exits successfully, because skipping an
# occurrence is what a schedule wants. The lock lives in the work area, so
# containers that share the work area are the ones that exclude each other.
#
# Environment:
#   MAPGEN_WORK_DIR  transient work area (default: /work); holds the lock file
#
# Usage:
#   mapgen-pass.sh [arguments of mapgen.sh]

set -u

WORK_DIR="${MAPGEN_WORK_DIR:-/work}"
LOCK_FILE="${WORK_DIR}/mapgen.lock"

# 75 is flock's own exit code for "could not acquire the lock" (-E), so it can
# be told apart from a pass that failed.
flock -n -E 75 "${LOCK_FILE}" /usr/local/bin/mapgen.sh "$@"
status=$?

if [ "${status}" = "75" ]; then
  echo "[mapgen] another pass holds ${LOCK_FILE}, skipping this trigger" >&2
  exit 0
fi

exit "${status}"
