#!/usr/bin/env bash
#
# mapgen.sh — regenerate OSM map databases from the imports manifest.
#
# One pass over all imports in the manifest, honoring per-id refresh
# frequency. Run under an external scheduler (cron, systemd timer,
# kubernetes CronJob); the script performs one pass and exits.
#
# Layout:
#   <repo>/public/names.json                                  region index (served)
#   <repo>/public/<region-index-path>/v<typeConfigVersion>/   served database slots
#   <repo>/private/admin/                                     script-owned records (not served)
#   <repo>/private/staging/                                   atomic replacement staging (not served)
#
# Integrity: source data is verified against the hash published by the
# download service (md5 sidecar). Output files carry CRC-32 (zlib-compatible)
# in db.json; POSIX `cksum` uses a different CRC and must NOT be used to
# verify them.
#
# Environment:
#   MAPGEN_CONFIG_DIR   directory with imports.json (default: /config)
#   MAPGEN_REPO_DIR     repository volume, contains public/ + private/ (default: /repository)
#   MAPGEN_PUBLIC_DIR   served root: names.json + database slots (default: <repo>/public)
#   MAPGEN_PRIVATE_DIR  script-owned records + staging (default: <repo>/private)
#   MAPGEN_NAMES_FILE   region index (default: <public>/names.json)
#   MAPGEN_WORK_DIR     transient work area (default: /work)
#   MAPGEN_IMPORT       import tool binary (default: Import)
#   MAPGEN_TYPEFILE     type definition file (default: map.ost)
#   MAPGEN_SCHEMA_VERSION  supported schema version (default: 1)
#
# Usage:
#   mapgen.sh [--check-config] [--help]

set -euo pipefail

CONFIG_DIR="${MAPGEN_CONFIG_DIR:-/config}"
REPO_DIR="${MAPGEN_REPO_DIR:-/repository}"
WORK_DIR="${MAPGEN_WORK_DIR:-/work}"
IMPORT_BIN="${MAPGEN_IMPORT:-Import}"
TYPEFILE="${MAPGEN_TYPEFILE:-map.ost}"
SCHEMA_VERSION="${MAPGEN_SCHEMA_VERSION:-1}"

# resolve to absolute paths so subshells that change directory stay correct
resolve_dir()
{
  case "$1" in
    /*) echo "$1" ;;
    *) echo "$(pwd)/$1" ;;
  esac
}

CONFIG_DIR=$(resolve_dir "$CONFIG_DIR")
REPO_DIR=$(resolve_dir "$REPO_DIR")
WORK_DIR=$(resolve_dir "$WORK_DIR")

PUBLIC_DIR=${MAPGEN_PUBLIC_DIR:-$REPO_DIR/public}
PRIVATE_DIR=${MAPGEN_PRIVATE_DIR:-$REPO_DIR/private}
PUBLIC_DIR=$(resolve_dir "$PUBLIC_DIR")
PRIVATE_DIR=$(resolve_dir "$PRIVATE_DIR")

IMPORTS_FILE="$CONFIG_DIR/imports.json"
NAMES_FILE=${MAPGEN_NAMES_FILE:-$PUBLIC_DIR/names.json}

ADMIN_DIR="$PRIVATE_DIR/admin"
STAGING_DIR="$PRIVATE_DIR/staging"

log() { echo "[mapgen] $*"; }
error() { echo "[mapgen] ERROR: $*" >&2; }
die() { error "$*"; exit 1; }

now_epoch() { date +%s; }

# --- configuration validation -------------------------------------------

validate_config()
{
  [ -f "$IMPORTS_FILE" ] || die "imports manifest not found: $IMPORTS_FILE"
  [ -f "$NAMES_FILE" ] || die "region index not found: $NAMES_FILE"

  jq -e ".schema == $SCHEMA_VERSION" "$IMPORTS_FILE" >/dev/null \
    || die "imports manifest schema not supported (expected $SCHEMA_VERSION)"
  jq -e ".schema == $SCHEMA_VERSION" "$NAMES_FILE" >/dev/null \
    || die "region index schema not supported (expected $SCHEMA_VERSION)"

  local duplicates
  duplicates=$(jq -r '[.imports[].id] | length - (unique | length)' "$IMPORTS_FILE")
  [ "$duplicates" = "0" ] || die "imports manifest contains duplicate ids"

  local manifest_ids leaf_ids
  manifest_ids=$(jq -r '.imports[].id' "$IMPORTS_FILE" | sort)
  leaf_ids=$(jq -r '
    def leaves:
      if has("children") then .children[] | leaves
      else .id end;
    .regions[] | leaves' "$NAMES_FILE" | sort)

  local missing_in_manifest missing_in_index
  missing_in_manifest=$(comm -23 <(printf '%s\n' "$leaf_ids") <(printf '%s\n' "$manifest_ids"))
  missing_in_index=$(comm -23 <(printf '%s\n' "$manifest_ids") <(printf '%s\n' "$leaf_ids"))

  [ -z "$missing_in_manifest" ] || die "region index leaves missing from imports manifest: $(echo $missing_in_manifest)"
  [ -z "$missing_in_index" ] || die "imports manifest ids missing from region index: $(echo $missing_in_index)"
}

# --- per-id settings -----------------------------------------------------

refresh_for()
{
  local id="$1" global_refresh="$2"
  jq -r --arg id "$id" --argjson g "$global_refresh" \
    '.imports[] | select(.id==$id) | (.refresh // $g)' "$IMPORTS_FILE"
}

history_for()
{
  local id="$1" global_history="$2"
  jq -r --arg id "$id" --argjson g "$global_history" \
    '.imports[] | select(.id==$id) | (.history // $g)' "$IMPORTS_FILE"
}

url_for()
{
  local id="$1"
  jq -r --arg id "$id" '.imports[] | select(.id==$id) | .url' "$IMPORTS_FILE"
}

region_path()
{
  local id="$1"
  jq -r --arg id "$id" '
    def find($target; $path):
      if .id == $target then $path
      elif has("children") then .children[] | find($target; $path + [.id])
      else empty end;
    .regions[] | find($id; [.id]) | join("/")' "$NAMES_FILE"
}

# --- script-owned state --------------------------------------------------

check_state_file() { echo "$ADMIN_DIR/$1_check.json"; }

read_last_checked()
{
  local id="$1" f
  f=$(check_state_file "$id")

  if [ -f "$f" ]; then
    jq -r '.lastCheckedAt // 0' "$f"
  else
    echo 0
  fi
}

write_check_state()
{
  local id="$1" checked_at="$2" hash="$3" f
  f=$(check_state_file "$id")

  mkdir -p "$ADMIN_DIR"
  jq -n --argjson t "$checked_at" --arg h "$hash" \
    '{lastCheckedAt: $t, lastSeenSourceMd5: $h}' > "$f.tmp"
  mv "$f.tmp" "$f"
}

newest_db_json()
{
  local path="$1"
  find "$PUBLIC_DIR/$path" -name db.json -printf '%T@ %p\n' 2>/dev/null \
    | sort -rn | head -1 | cut -d' ' -f2-
}

# --- one import ----------------------------------------------------------

process_import()
{
  local id="$1" global_refresh="$2" global_history="$3"
  local refresh history url path
  local now last_checked published_hash newest previous_md5
  local pbf out_dir version target staging new_dir kept pruned changed

  refresh=$(refresh_for "$id" "$global_refresh")
  history=$(history_for "$id" "$global_history")
  url=$(url_for "$id")
  path=$(region_path "$id")

  [ -n "$path" ] || die "no region index path for id '$id'"

  now=$(now_epoch)
  last_checked=$(read_last_checked "$id")

  if [ $((now - last_checked)) -lt $((refresh * 86400)) ]; then
    log "$id: not due (last checked $last_checked, refresh $refresh days)"
    return 0
  fi

  log "$id: checking for changes"

  if ! published_hash=$(curl -fsSL "$url.md5" 2>/dev/null | awk '{print $1}'); then
    error "$id: cannot fetch published hash from $url.md5"
    return 1
  fi

  [ -n "$published_hash" ] || { error "$id: empty published hash"; return 1; }

  write_check_state "$id" "$now" "$published_hash"

  newest=$(newest_db_json "$path")
  previous_md5=""
  if [ -n "$newest" ]; then
    previous_md5=$(jq -r '.source.md5 // ""' "$newest")
  fi

  if [ -n "$previous_md5" ] && [ "$previous_md5" = "$published_hash" ]; then
    log "$id: source unchanged"
    return 0
  fi

  log "$id: source changed, downloading"
  pbf="$WORK_DIR/$id.${url##*.}"

  if ! curl -fsSL "$url" -o "$pbf"; then
    error "$id: download failed"
    return 1
  fi

  # The download service (e.g. Geofabrik) publishes an md5 sidecar file;
  # md5 is used here for download integrity only, not for security, and
  # no stronger provider hash exists. See sonar-project.properties
  # (shell:S4790) for the justification.
  if ! (cd "$WORK_DIR" && printf '%s  %s\n' "$published_hash" "$pbf" | md5sum -c - >/dev/null 2>&1); then
    error "$id: downloaded source failed verification"
    return 1
  fi

  out_dir="$WORK_DIR/$id-out"
  rm -rf "$out_dir"
  mkdir -p "$out_dir"

  log "$id: importing"
  if ! "$IMPORT_BIN" --typefile "$TYPEFILE" \
       --destinationDirectory "$out_dir" \
       --source-url "$url" --source-md5 "$published_hash" \
       "$pbf" 2>&1 | tee "$WORK_DIR/$id-import.log"; then
    error "$id: import failed (see $WORK_DIR/$id-import.log)"
    return 1
  fi

  [ -f "$out_dir/db.json" ] || { error "$id: import produced no db.json"; return 1; }

  version=$(jq -r '.typeConfigVersion' "$out_dir/db.json")

  target="$PUBLIC_DIR/$path/v$version"
  staging="$STAGING_DIR/$id"
  new_dir="$staging/v$version.new"

  rm -rf "$new_dir"
  mkdir -p "$new_dir"

  while IFS= read -r fname; do
    cp "$out_dir/$fname" "$new_dir/"
  done < <(jq -r '.output.files | keys[]' "$out_dir/db.json")
  cp "$out_dir/db.json" "$new_dir/"

  mkdir -p "$(dirname "$target")"

  if [ -d "$target" ]; then
    mv "$target" "$staging/v$version.old"
  fi
  mv "$new_dir" "$target"
  rm -rf "$staging/v$version.old"

  log "$id: placed at $path/v$version"

  if [ "$history" -gt 0 ]; then
    kept=$(ls -d "$PUBLIC_DIR/$path"/v* 2>/dev/null | sort -V | head -n -"$history" || true)
  else
    kept=$(ls -d "$PUBLIC_DIR/$path"/v* 2>/dev/null || true)
  fi

  pruned=""
  if [ -n "$kept" ]; then
    pruned=$(basename -a $kept 2>/dev/null | tr '\n' ' ' | sed 's/ $//')
    log "$id: pruning $pruned"
    rm -rf $kept
  fi

  changed=true
  if [ -n "$previous_md5" ] && [ "$previous_md5" = "$published_hash" ]; then
    changed=false
  fi

  mkdir -p "$ADMIN_DIR"
  jq -n --arg id "$id" \
     --argjson v "$version" \
     --arg generatedAt "$(jq -r '.generatedAt' "$out_dir/db.json")" \
     --arg placedAt "$(date -u +%Y-%m-%dT%H:%M:%SZ)" \
     --arg path "$path" \
     --argjson history "$history" \
     --arg previousSourceMd5 "$previous_md5" \
     --argjson sourceChanged "$changed" \
     --arg pruned "$pruned" \
     '{importId: $id, typeConfigVersion: $v, generatedAt: $generatedAt, placedAt: $placedAt, history: $history, serverPath: $path, previousSourceMd5: $previousSourceMd5, sourceChanged: $sourceChanged, pruned: ($pruned | split(" ") | map(select(. != "")))}' \
     > "$ADMIN_DIR/${id}_v${version}_generation.json.tmp"
  mv "$ADMIN_DIR/${id}_v${version}_generation.json.tmp" "$ADMIN_DIR/${id}_v${version}_generation.json"

  return 0
}

# --- main ----------------------------------------------------------------

main()
{
  local global_refresh global_history ids failed=0 id

  mkdir -p "$PUBLIC_DIR"
  validate_config

  global_refresh=$(jq -r '.refresh // 7' "$IMPORTS_FILE")
  global_history=$(jq -r '.history // 2' "$IMPORTS_FILE")

  mkdir -p "$PUBLIC_DIR" "$ADMIN_DIR" "$STAGING_DIR" "$WORK_DIR"

  ids=$(jq -r '.imports[].id' "$IMPORTS_FILE")

  for id in $ids; do
    if ! process_import "$id" "$global_refresh" "$global_history"; then
      failed=1
    fi
  done

  if [ "$failed" = "1" ]; then
    error "one or more imports failed"
    exit 1
  fi

  log "done"
}

case "${1:-}" in
  --help|-h)
    sed -n '2,30p' "$0" | sed 's/^# \{0,1\}//'
    exit 0
    ;;
  --check-config)
    validate_config
    echo "config OK"
    exit 0
    ;;
  *)
    main
    ;;
esac
