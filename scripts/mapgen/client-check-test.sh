#!/usr/bin/env bash
#
# client-check-test.sh — decision-matrix test harness for the client
# update check contract (specs/client-update-check).
#
# Simulates the client behavior against fixture repositories:
#   - probe only the client's own type-config version
#   - compare generatedAt against the local baseline
#   - fresh install offer, missing-data handling
#   - downloaded file verification against db.json checksums
#
# Exits non-zero on any failed assertion.

set -euo pipefail

command -v jq >/dev/null 2>&1 \
  || { echo "client-check-test.sh: jq not found" >&2; exit 1; }
command -v python3 >/dev/null 2>&1 \
  || { echo "client-check-test.sh: python3 not found" >&2; exit 1; }

TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

# --- path helpers ------------------------------------------------------

# MSYS/git-bash paths (e.g. /tmp/...) are not understood by a native
# Windows python3. Convert with cygpath when available so python opens
# exactly the files bash just wrote.
python_path()
{
  if command -v cygpath >/dev/null 2>&1; then
    cygpath -w "$1"
  else
    printf '%s' "$1"
  fi
}

PROBE_LOG="$TMP/probes.log"
: > "$PROBE_LOG"

# --- fixture helpers ------------------------------------------------------

make_db_json()
{
  local dir="$1" version="$2" generated_at="$3"
  mkdir -p "$dir"
  printf 'map.lib!' > "$dir/map.lib"
  local pd crc
  pd=$(python_path "$dir")
  crc=$(python3 -c "import zlib; print(zlib.crc32(open('$pd/map.lib','rb').read()) & 0xffffffff)")
  cat > "$dir/db.json" <<EOF
{
  "schema": 1,
  "typeConfigVersion": $version,
  "generatedAt": "$generated_at",
  "output": {
    "files": {
      "map.lib": {"size": 8, "crc32": $crc}
    }
  }
}
EOF
}

# --- client simulation ----------------------------------------------------
#
# client_check <repo> <client_version> <local_generated_at|empty>
#   echoes one of: update-available | up-to-date | available | unavailable
#   appends the probed path to $PROBE_LOG

client_check()
{
  local repo="$1" client_version="$2" local_at="$3"
  local probe="$repo/berlin/v$client_version/db.json"

  echo "$probe" >> "$PROBE_LOG"

  if [ ! -f "$probe" ]; then
    echo "unavailable"
    return
  fi

  local remote_at
  remote_at=$(jq -r '.generatedAt' "$probe")

  if [ -z "$local_at" ]; then
    echo "available"
    return
  fi

  if [ "$remote_at" \> "$local_at" ]; then
    echo "update-available"
  else
    echo "up-to-date"
  fi
}

# --- assertions -----------------------------------------------------------

assert_eq()
{
  local expected="$1" actual="$2" label="$3"
  if [ "$expected" != "$actual" ]; then
    echo "FAIL: $label: expected '$expected', got '$actual'" >&2
    exit 1
  fi
  echo "ok: $label"
}

# --- test cases -----------------------------------------------------------

# 1. newer server data -> update available
REPO="$TMP/repo1"
make_db_json "$REPO/berlin/v27" 27 "2026-09-07T10:00:00Z"
assert_eq "update-available" "$(client_check "$REPO" 27 "2026-09-01T10:00:00Z")" \
  "newer server data reported as update"

# 2. equal server data -> up to date
assert_eq "up-to-date" "$(client_check "$REPO" 27 "2026-09-07T10:00:00Z")" \
  "equal server data reported as up to date"

# 3. older server data -> up to date
assert_eq "up-to-date" "$(client_check "$REPO" 27 "2026-09-08T10:00:00Z")" \
  "older server data reported as up to date"

# 4. fresh client (no local data) -> available for installation
assert_eq "available" "$(client_check "$REPO" 27 "")" \
  "fresh client offered the database"

# 5. no data for the client's version -> unavailable
assert_eq "unavailable" "$(client_check "$REPO" 25 "2026-09-01T10:00:00Z")" \
  "missing data for own version reported as unavailable"

# 6. own-version probe only: client v27 must never probe v28
REPO2="$TMP/repo2"
make_db_json "$REPO2/berlin/v27" 27 "2026-09-07T10:00:00Z"
make_db_json "$REPO2/berlin/v28" 28 "2026-09-08T10:00:00Z"
: > "$PROBE_LOG"
client_check "$REPO2" 27 "2026-09-01T10:00:00Z" > /dev/null
if grep -q "v28" "$PROBE_LOG"; then
  echo "FAIL: client probed a newer type-config version" >&2
  exit 1
fi
echo "ok: client never probes a newer type-config version"

# 7. newer version only on server -> client v25 gets unavailable, no v26/v27 probe
REPO3="$TMP/repo3"
make_db_json "$REPO3/berlin/v27" 27 "2026-09-07T10:00:00Z"
: > "$PROBE_LOG"
assert_eq "unavailable" "$(client_check "$REPO3" 25 "2026-09-01T10:00:00Z")" \
  "newer-only server data not offered to older client"
if grep -q "v26\|v27" "$PROBE_LOG"; then
  echo "FAIL: client probed a version other than its own" >&2
  exit 1
fi
echo "ok: client with newer-only server data probes only its own version"

# 8. downloaded file verification: corrupt file rejected
REPO4="$TMP/repo4"
make_db_json "$REPO4/berlin/v27" 27 "2026-09-07T10:00:00Z"
printf 'corrupt!' > "$REPO4/berlin/v27/map.lib"
if python3 - "$(python_path "$REPO4/berlin/v27")" <<'EOF'
import json, sys, zlib
d = json.load(open(sys.argv[1] + "/db.json"))
for name, meta in d["output"]["files"].items():
    data = open(sys.argv[1] + "/" + name, "rb").read()
    if len(data) != meta["size"] or (zlib.crc32(data) & 0xffffffff) != meta["crc32"]:
        sys.exit(1)
sys.exit(0)
EOF
then
  echo "FAIL: corrupt file passed verification" >&2
  exit 1
fi
echo "ok: corrupt downloaded file rejected by checksum verification"

# 9. intact file passes verification
REPO5="$TMP/repo5"
make_db_json "$REPO5/berlin/v27" 27 "2026-09-07T10:00:00Z"
if ! python3 - "$(python_path "$REPO5/berlin/v27")" <<'EOF'
import json, sys, zlib
d = json.load(open(sys.argv[1] + "/db.json"))
for name, meta in d["output"]["files"].items():
    data = open(sys.argv[1] + "/" + name, "rb").read()
    if len(data) != meta["size"] or (zlib.crc32(data) & 0xffffffff) != meta["crc32"]:
        sys.exit(1)
sys.exit(0)
EOF
then
  echo "FAIL: intact file failed verification" >&2
  exit 1
fi
echo "ok: intact file passes checksum verification"

echo "all client update check tests passed"
