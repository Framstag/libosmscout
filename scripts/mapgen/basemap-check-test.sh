#!/usr/bin/env bash
#
# basemap-check-test.sh — contract test for scripts/mapgen/mapgen-basemap.sh.
#
# Runs the basemap step against stub import tools, so the behaviour it checks is
# the decision and placement logic of the step itself: production, the file set
# a client requires, slot placement, the availability manifest, the check state,
# the refresh and adoption gates, retention, staging recovery, and that a
# failing step leaves the served basemap alone.
#
# The real tools are not exercised here; that needs a planet export and world
# coastline data.
#
# Requirements: bash, jq, curl, unzip, md5sum, and zip for the fixture archive.
#
# Usage:
#   basemap-check-test.sh

set -u

SCRIPT_DIR=$(dirname "$(readlink -f "$0")")
BASEMAP_SCRIPT="$SCRIPT_DIR/mapgen-basemap.sh"

command -v jq >/dev/null || { echo "basemap-check-test: jq is required" >&2; exit 1; }
command -v curl >/dev/null || { echo "basemap-check-test: curl is required" >&2; exit 1; }
command -v unzip >/dev/null || { echo "basemap-check-test: unzip is required" >&2; exit 1; }
command -v zip >/dev/null || { echo "basemap-check-test: skipped (zip is required to build the fixture archive)"; exit 0; }

ROOT=$(mktemp -d)
trap 'rm -rf "$ROOT"' EXIT

REPO="$ROOT/repo"
CONFIG="$ROOT/config"
WORK="$ROOT/work"
TOOLS="$ROOT/tools"

mkdir -p "$REPO/public" "$REPO/private" "$CONFIG" "$WORK" "$TOOLS" "$ROOT/shape"

FAILURES=0

check() {
  local name="$1" expected="$2" actual="$3"

  if [[ "$expected" = "$actual" ]]; then
    echo "ok      : $name"
  else
    echo "FAIL    : $name (expected '$expected', got '$actual')"
    FAILURES=$((FAILURES + 1))
  fi
}

check_true() {
  local name="$1" condition="$2"

  check "$name" "yes" "$([[ "$condition" = "0" ]] && echo yes || echo no)"
}

# --- inputs -----------------------------------------------------------------

printf 'planet-export' > "$CONFIG/planet.osm.pbf"
printf 'shape' > "$ROOT/shape/coastlines.shp"
(cd "$ROOT/shape" && zip -q "$ROOT/coastlines.zip" coastlines.shp)

cat > "$CONFIG/basemap.json" <<EOF
{
  "schema": 1,
  "refresh": 7,
  "coastlinesRefresh": 90,
  "history": 2,
  "extract": "$CONFIG/planet.osm.pbf",
  "coastlines": { "url": "file://$ROOT/coastlines.zip" },
  "importOptions": { "waterIndexMinMag": 6, "waterIndexMaxMag": 6, "langOrder": "en,#" }
}
EOF

# --- stub tools -------------------------------------------------------------

cat > "$TOOLS/Import" <<'STUB'
#!/usr/bin/env bash
# stub: fills the destination directory with the file set a client requires
set -eu
dest=""
while [[ $# -gt 0 ]]; do
  case "$1" in
    --destinationDirectory) dest="$2"; shift 2;;
    *) shift;;
  esac
done
mkdir -p "$dest"
REQUIRED="bounding.dat nodes.dat areas.dat ways.dat areanode.idx areaarea.idx areaway.idx areasopt.dat waysopt.dat location.idx water.idx intersections.dat intersections.idx route.dat arearoute.idx router.dat router2.dat types.dat"
for f in $REQUIRED; do
  printf 'data-%s' "$f" > "$dest/$f"
done
# the real import tool writes an index of its own; the basemap step replaces it
rm -f "$dest/water.idx"
files="{}"
for f in $REQUIRED; do
  [[ -f "$dest/$f" ]] || continue
  files=$(jq -c --arg n "$f" --argjson s "$(stat -c %s "$dest/$f")" '. + {($n): {size: $s, crc32: 1}}' <<<"$files")
done
jq -n --argjson files "$files" \
  '{schema: 1, typeConfigVersion: 27, generatedAt: "2026-09-07T10:00:00Z", import: {tool: "Import", version: "test"}, output: {boundingBox: {minLon: -180, minLat: -90, maxLon: 180, maxLat: 90}, files: $files}, stats: {types: 3}}' \
  > "$dest/db.json"
STUB

cat > "$TOOLS/BasemapImport" <<'STUB'
#!/usr/bin/env bash
# stub: writes the water index and adds it to the inventory, like the real tool
#
# Like the real tool it rejects options it does not know, so an option that the
# pass sends to the wrong tool is a failing stub rather than a silent no-op.
set -eu
dest=""
while [[ $# -gt 0 ]]; do
  case "$1" in
    --destinationDirectory)
      dest="$2"; shift 2;;
    --coastlines|--minIndexLevel|--maxIndexLevel|--maxWaterDistance)
      shift 2;;
    --*)
      echo "BasemapImport stub: unknown option: $1" >&2
      exit 1;;
    *)
      shift;;
  esac
done
mkdir -p "$dest"
printf 'water-index-data' > "$dest/water.idx"
tmp=$(mktemp)
jq --argjson s "$(stat -c %s "$dest/water.idx")" \
   '.output.files["water.idx"] = {size: $s, crc32: 2}' "$dest/db.json" > "$tmp"
mv "$tmp" "$dest/db.json"
STUB

chmod +x "$TOOLS/Import" "$TOOLS/BasemapImport"

run_step() {
  MAPGEN_CONFIG_DIR="$CONFIG" \
  MAPGEN_REPO_DIR="$REPO" \
  MAPGEN_WORK_DIR="$WORK" \
  MAPGEN_IMPORT="$TOOLS/Import" \
  MAPGEN_BASEMAP_IMPORT="$TOOLS/BasemapImport" \
  MAPGEN_BASEMAP_TYPEFILE="$TOOLS/basemap.ost" \
  MAPGEN_BASEMAP_FILE="$CONFIG/basemap.json" \
  "$BASEMAP_SCRIPT" "$@"
}

recheck_due() {
  jq --argjson t "$(($(date +%s) - $1 * 86400))" '.lastCheckedAt = $t' \
    "$REPO/private/admin/basemap_check.json" > "$ROOT/state.tmp"
  mv "$ROOT/state.tmp" "$REPO/private/admin/basemap_check.json"
}

state_value() {
  jq -r "$1" "$REPO/private/admin/basemap_check.json"
}

echo "== pass 1: nothing served yet"

if ! run_step > "$ROOT/pass1.log" 2>&1; then
  echo "FAIL    : first pass exited non-zero"
  tail -5 "$ROOT/pass1.log"
  FAILURES=$((FAILURES + 1))
fi

check "one slot served" "1" "$(ls "$REPO/public/basemap" | grep -c '^v')"
check "slot named by the database format version" "v27" "$(ls "$REPO/public/basemap" | grep '^v' | head -1)"
check "slot holds the database file set and its metadata" "19" "$(ls "$REPO/public/basemap/v27" | wc -l | tr -d ' ')"
check "water index is the coastline-derived one" "water-index-data" "$(cat "$REPO/public/basemap/v27/water.idx")"
check "manifest names the version" "27" "$(jq -r '.versions[0].typeConfigVersion' "$REPO/public/basemap/index.json")"
check "manifest names the change time" "2026-09-07T10:00:00Z" "$(jq -r '.versions[0].changedAt' "$REPO/public/basemap/index.json")"
check "manifest was written as a whole" "no" "$([[ -f "$REPO/public/basemap/index.json.tmp" ]] && echo yes || echo no)"
check "generation record written" "1" "$(ls "$REPO/private/admin" | grep -c generation)"
check "check state records the extract content" "$(md5sum "$CONFIG/planet.osm.pbf" | awk '{print $1}')" "$(state_value '.extractMd5')"
check "staging left empty" "0" "$(ls -A "$REPO/private/staging/basemap" 2>/dev/null | wc -l | tr -d ' ')"

echo "== pass 2: refresh elapsed, nothing changed"

recheck_due 8
run_step > "$ROOT/pass2.log" 2>&1 || { echo "FAIL    : second pass exited non-zero"; tail -5 "$ROOT/pass2.log"; FAILURES=$((FAILURES + 1)); }

check "no additional generation record" "1" "$(ls "$REPO/private/admin" | grep -c generation)"
check_true "no change reported" "$(grep -q 'no input changed' "$ROOT/pass2.log"; echo $?)"

echo "== pass 3: inside the refresh window"

run_step > "$ROOT/pass3.log" 2>&1 || { echo "FAIL    : third pass exited non-zero"; tail -5 "$ROOT/pass3.log"; FAILURES=$((FAILURES + 1)); }

check_true "not due reported" "$(grep -q 'not due' "$ROOT/pass3.log"; echo $?)"

echo "== pass 4: newer coastline published, adoption not due"

printf 'shape-v2' > "$ROOT/shape/coastlines.shp"
rm -f "$ROOT/coastlines.zip"
(cd "$ROOT/shape" && zip -q "$ROOT/coastlines.zip" coastlines.shp)

recheck_due 8
run_step > "$ROOT/pass4.log" 2>&1 || { echo "FAIL    : fourth pass exited non-zero"; tail -5 "$ROOT/pass4.log"; FAILURES=$((FAILURES + 1)); }

check "no additional generation record" "1" "$(ls "$REPO/private/admin" | grep -c generation)"
check_true "not due for adoption reported" "$(grep -q 'not due for adoption' "$ROOT/pass4.log"; echo $?)"
check "served water index unchanged" "water-index-data" "$(cat "$REPO/public/basemap/v27/water.idx")"

echo "== pass 5: adoption due"

jq --argjson t "$(($(date +%s) - 91 * 86400))" '.lastCheckedAt = $t | .lastAdoptedAt = $t' \
  "$REPO/private/admin/basemap_check.json" > "$ROOT/state.tmp"
mv "$ROOT/state.tmp" "$REPO/private/admin/basemap_check.json"

run_step > "$ROOT/pass5.log" 2>&1 || { echo "FAIL    : fifth pass exited non-zero"; tail -5 "$ROOT/pass5.log"; FAILURES=$((FAILURES + 1)); }

check "no additional generation record" "1" "$(ls "$REPO/private/admin" | grep -c generation)"
check_true "adoption reported" "$(grep -q 'adopted newer coastline data' "$ROOT/pass5.log"; echo $?)"

echo "== pass 6: a new database format version gets its own slot"

sed -i 's/typeConfigVersion: 27/typeConfigVersion: 28/' "$TOOLS/Import"
printf 'planet-export-v2' > "$CONFIG/planet.osm.pbf"

recheck_due 8
run_step > "$ROOT/pass6.log" 2>&1 || { echo "FAIL    : sixth pass exited non-zero"; tail -5 "$ROOT/pass6.log"; FAILURES=$((FAILURES + 1)); }

check "two slots served" "2" "$(ls "$REPO/public/basemap" | grep -c '^v')"
check "manifest names both versions" "2" "$(jq -r '.versions | length' "$REPO/public/basemap/index.json")"

echo "== pass 7: retention prunes the oldest version"

jq '.history=1' "$CONFIG/basemap.json" > "$ROOT/config.tmp"
mv "$ROOT/config.tmp" "$CONFIG/basemap.json"
printf 'planet-export-v3' > "$CONFIG/planet.osm.pbf"

recheck_due 8
run_step > "$ROOT/pass7.log" 2>&1 || { echo "FAIL    : seventh pass exited non-zero"; tail -5 "$ROOT/pass7.log"; FAILURES=$((FAILURES + 1)); }

check "one slot served after pruning" "1" "$(ls "$REPO/public/basemap" | grep -c '^v')"
check "manifest names only what is served" "1" "$(jq -r '.versions | length' "$REPO/public/basemap/index.json")"
check_true "pruning reported" "$(grep -q 'pruning basemap versions' "$ROOT/pass7.log"; echo $?)"

check "one cached import output kept" "1" "$(ls -d "$WORK"/basemap/import-* 2>/dev/null | wc -l | tr -d ' ')"

echo "== recovery: an interrupted replacement is closed on the next pass"

mkdir -p "$REPO/private/staging/basemap/v27.old"
printf 'restored' > "$REPO/private/staging/basemap/v27.old/types.dat"

run_step > "$ROOT/pass8.log" 2>&1 || true

check_true "slot restored from staging" "$([[ -f "$REPO/public/basemap/v27/types.dat" ]] && echo 0 || echo 1)"
check_true "restore reported" "$(grep -q 'restored v27' "$ROOT/pass8.log"; echo $?)"

echo "== failure isolation: a failing step leaves the served basemap alone"

SERVED_VERSION=$(jq -r '.versions[-1].typeConfigVersion' "$REPO/public/basemap/index.json")
SERVED_STATE=$(state_value '.extractMd5')

cp "$TOOLS/BasemapImport" "$TOOLS/BasemapImport.good"
printf '#!/usr/bin/env bash\nexit 1\n' > "$TOOLS/BasemapImport"
chmod +x "$TOOLS/BasemapImport"

printf 'planet-export-v4' > "$CONFIG/planet.osm.pbf"

recheck_due 8

if run_step > "$ROOT/pass9.log" 2>&1; then
  echo "FAIL    : a failing step reported success"
  FAILURES=$((FAILURES + 1))
else
  echo "ok      : failing step reported failure"
fi

check_true "served slot still complete" "$([[ -f "$REPO/public/basemap/v$SERVED_VERSION/types.dat" && -f "$REPO/public/basemap/v$SERVED_VERSION/water.idx" ]] && echo 0 || echo 1)"
check "manifest unchanged" "1" "$(jq -r '.versions | length' "$REPO/public/basemap/index.json")"
check "failed production did not record the new input" "$SERVED_STATE" "$(state_value '.extractMd5')"

cp "$TOOLS/BasemapImport.good" "$TOOLS/BasemapImport"

echo "== incomplete database is rejected before anything is placed"

cp "$TOOLS/Import" "$TOOLS/Import.good"
sed -i 's/ router2.dat / /' "$TOOLS/Import"

printf 'planet-export-v5' > "$CONFIG/planet.osm.pbf"

recheck_due 8

SLOTS_BEFORE=$(ls "$REPO/public/basemap" | grep -c '^v')

if run_step > "$ROOT/pass10.log" 2>&1; then
  echo "FAIL    : an incomplete database was accepted"
  FAILURES=$((FAILURES + 1))
else
  echo "ok      : incomplete database rejected"
fi

check_true "missing file named" "$(grep -q 'missing files a client requires: router2.dat' "$ROOT/pass10.log"; echo $?)"
check "no new slot placed" "$SLOTS_BEFORE" "$(ls "$REPO/public/basemap" | grep -c '^v')"

cp "$TOOLS/Import.good" "$TOOLS/Import"

echo

if [[ "$FAILURES" -gt 0 ]]; then
  echo "basemap-check-test: $FAILURES check(s) failed"
  exit 1
fi

echo "basemap-check-test: all checks passed"
