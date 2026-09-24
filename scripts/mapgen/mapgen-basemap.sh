#!/usr/bin/env bash
#
# mapgen-basemap.sh — regenerate the world basemap of the map repository.
#
# One step inside a mapgen pass (see mapgen.sh): it decides from the content of
# its inputs whether a new basemap is needed, produces it, places it into a
# version-keyed slot of the served tree and updates the availability manifest
# clients read. Regional databases are not touched, and a basemap already being
# served survives any failure here.
#
# Inputs:
#   - a pre-filtered planet export, named by the basemap configuration and read
#     from the configuration area (mounted read-only)
#   - world coastline data, fetched from the configured - or image-provided -
#     location, checked with a conditional request and adopted only once the
#     adoption interval has elapsed
#
# Layout (inside the repository):
#   <repo>/public/basemap/index.json                      availability manifest (served)
#   <repo>/public/basemap/v<typeConfigVersion>/           served database slot
#   <repo>/private/admin/basemap_check.json               script-owned check state
#   <repo>/private/admin/basemap_v<N>_generation.json     script-owned generation record
#   <repo>/private/staging/basemap/                       atomic replacement staging
#
# Environment:
#   MAPGEN_CONFIG_DIR        directory with basemap.json (default: /config)
#   MAPGEN_BASEMAP_FILE      basemap configuration (default: <config>/basemap.json)
#   MAPGEN_REPO_DIR          repository volume (default: /repository)
#   MAPGEN_PUBLIC_DIR        served root (default: <repo>/public)
#   MAPGEN_PRIVATE_DIR       script-owned records + staging (default: <repo>/private)
#   MAPGEN_WORK_DIR          transient work area (default: /work)
#   MAPGEN_IMPORT            import tool binary (default: Import)
#   MAPGEN_BASEMAP_IMPORT    basemap import tool binary (default: BasemapImport)
#   MAPGEN_BASEMAP_TYPEFILE  basemap type definition file (default: basemap.ost)
#   MAPGEN_UNZIP             unzip binary (default: unzip)
#   MAPGEN_COASTLINES_URL    default coastline source (the image value)
#   MAPGEN_SCHEMA_VERSION    supported configuration schema version (default: 1)
#   MAPGEN_CONNECT_TIMEOUT, MAPGEN_DOWNLOAD_ATTEMPTS, MAPGEN_DOWNLOAD_RETRY_WAIT
#                            recovery of failing downloads
#
# Usage:
#   mapgen-basemap.sh [--check-config] [--help]

set -euo pipefail

CONFIG_DIR="${MAPGEN_CONFIG_DIR:-/config}"
REPO_DIR="${MAPGEN_REPO_DIR:-/repository}"
WORK_DIR="${MAPGEN_WORK_DIR:-/work}"
IMPORT_BIN="${MAPGEN_IMPORT:-Import}"
BASEMAP_IMPORT_BIN="${MAPGEN_BASEMAP_IMPORT:-BasemapImport}"
TYPEFILE="${MAPGEN_BASEMAP_TYPEFILE:-basemap.ost}"
UNZIP_BIN="${MAPGEN_UNZIP:-unzip}"
SCHEMA_VERSION="${MAPGEN_SCHEMA_VERSION:-1}"
DEFAULT_COASTLINES_URL="${MAPGEN_COASTLINES_URL:-}"

CONNECT_TIMEOUT="${MAPGEN_CONNECT_TIMEOUT:-20}"
DOWNLOAD_ATTEMPTS="${MAPGEN_DOWNLOAD_ATTEMPTS:-3}"
DOWNLOAD_RETRY_WAIT="${MAPGEN_DOWNLOAD_RETRY_WAIT:-30}"

# resolve to absolute paths so subshells that change directory stay correct
resolve_dir()
{
  local path="$1"
  case "$path" in
    /*) echo "$path" ;;
    *) echo "$(pwd)/$path" ;;
  esac
}

CONFIG_DIR=$(resolve_dir "$CONFIG_DIR")
REPO_DIR=$(resolve_dir "$REPO_DIR")
WORK_DIR=$(resolve_dir "$WORK_DIR")

PUBLIC_DIR=${MAPGEN_PUBLIC_DIR:-$REPO_DIR/public}
PRIVATE_DIR=${MAPGEN_PRIVATE_DIR:-$REPO_DIR/private}
PUBLIC_DIR=$(resolve_dir "$PUBLIC_DIR")
PRIVATE_DIR=$(resolve_dir "$PRIVATE_DIR")

BASEMAP_FILE=${MAPGEN_BASEMAP_FILE:-$CONFIG_DIR/basemap.json}

ADMIN_DIR="$PRIVATE_DIR/admin"
STAGING_DIR="$PRIVATE_DIR/staging/basemap"
BASEMAP_PUBLIC_DIR="$PUBLIC_DIR/basemap"
MANIFEST="$BASEMAP_PUBLIC_DIR/index.json"
CHECK_STATE="$ADMIN_DIR/basemap_check.json"

WORK_BASEMAP_DIR="$WORK_DIR/basemap"
# Two coastline archives: the one that was adopted and is used for production,
# and the newest one fetched from the source, which only becomes the adopted one
# once the adoption interval has elapsed.
COASTLINES_ZIP="$WORK_BASEMAP_DIR/coastlines.zip"
COASTLINES_CANDIDATE="$WORK_BASEMAP_DIR/coastlines.candidate.zip"
COASTLINES_HEADERS="$WORK_BASEMAP_DIR/coastlines.headers"
COASTLINES_DIR="$WORK_BASEMAP_DIR/coastlines"
COASTLINES_SHAPE="$COASTLINES_DIR/coastlines.shp"
OUT_DIR="$WORK_BASEMAP_DIR/out"

# The file set a client requires of a database directory (MapDirectory). A
# missing file aborts a client download, so the produced basemap is checked
# against this list before anything is placed.
REQUIRED_FILES="bounding.dat nodes.dat areas.dat ways.dat \
areanode.idx areaarea.idx areaway.idx areasopt.dat waysopt.dat \
location.idx water.idx intersections.dat intersections.idx \
route.dat arearoute.idx router.dat router2.dat types.dat"

log() { echo "[mapgen-basemap] $*"; }
error() { echo "[mapgen-basemap] ERROR: $*" >&2; }
die() { error "$*"; exit 1; }

now_epoch() { date +%s; }

# --- configuration validation -------------------------------------------

# The set of configuration keys that may be passed on to the tools. Anything
# else is a typo, and a typo that silently does nothing is worse than a
# configuration error.
IMPORT_OPTION_NUMBERS="waterIndexMinMag waterIndexMaxMag lowZoomOptMaxMag areaNodeGridMag maxAdminLevel minIndexLevel maxIndexLevel"
IMPORT_OPTION_STRINGS="langOrder altLangOrder"
IMPORT_OPTION_BOOLEANS="strictAreas"
IMPORT_OPTION_COUNTS="maxWaterDistance"

# Which tool receives which option. The import tool takes the first row, the
# water index tool the second; passing an option to the tool that does not know
# it makes that tool refuse to run at all.

IMPORT_TOOL_OPTIONS="waterIndexMinMag waterIndexMaxMag lowZoomOptMaxMag areaNodeGridMag maxAdminLevel langOrder altLangOrder strictAreas"
BASEMAP_TOOL_OPTIONS="minIndexLevel maxIndexLevel maxWaterDistance"

validate_config()
{
  [[ -f "$BASEMAP_FILE" ]] || die "basemap configuration not found: $BASEMAP_FILE"

  jq -e ".schema == $SCHEMA_VERSION" "$BASEMAP_FILE" >/dev/null \
    || die "basemap configuration schema not supported (expected $SCHEMA_VERSION)"

  jq -e '.extract | type == "string" and length > 0' "$BASEMAP_FILE" >/dev/null \
    || die "basemap configuration: 'extract' must name the planet export"

  jq -e '.refresh | type == "number" and . > 0 and . == floor' "$BASEMAP_FILE" >/dev/null \
    || die "basemap configuration: 'refresh' must be a positive whole number of days"

  jq -e '.coastlinesRefresh | type == "number" and . > 0 and . == floor' "$BASEMAP_FILE" >/dev/null \
    || die "basemap configuration: 'coastlinesRefresh' must be a positive whole number of days"

  jq -e '.history | type == "number" and . >= 0 and . == floor' "$BASEMAP_FILE" >/dev/null \
    || die "basemap configuration: 'history' must be a whole number of versions (0 keeps all)"

  local refresh adoption
  refresh=$(jq -r '.refresh' "$BASEMAP_FILE")
  adoption=$(jq -r '.coastlinesRefresh' "$BASEMAP_FILE")

  if [[ "$adoption" -lt "$refresh" ]]; then
    die "basemap configuration: 'coastlinesRefresh' ($adoption) is shorter than 'refresh' ($refresh)"
  fi

  jq -e '(.coastlines // {}) | type == "object"' "$BASEMAP_FILE" >/dev/null \
    || die "basemap configuration: 'coastlines' must be an object"

  jq -e 'if (.coastlines // {} | has("url")) then ((.coastlines.url | type) == "string" and (.coastlines.url | length) > 0) else true end' "$BASEMAP_FILE" >/dev/null \
    || die "basemap configuration: 'coastlines.url' must be a non-empty location"

  jq -e 'if (.coastlines // {} | has("sha256")) then ((.coastlines.sha256 | type) == "string" and (.coastlines.sha256 | test("^[0-9a-fA-F]{64}$"))) else true end' "$BASEMAP_FILE" >/dev/null \
    || die "basemap configuration: 'coastlines.sha256' must be a 64 character hexadecimal checksum"

  validate_import_options
}

validate_import_options()
{
  jq -e '(.importOptions // {}) | type == "object"' "$BASEMAP_FILE" >/dev/null \
    || die "basemap configuration: 'importOptions' must be an object"

  local allowed unknown key

  allowed="$IMPORT_OPTION_NUMBERS $IMPORT_OPTION_STRINGS $IMPORT_OPTION_BOOLEANS $IMPORT_OPTION_COUNTS"

  unknown=$(jq -r --arg allowed "$allowed" \
    '[.importOptions // {} | keys[] | select(. as $k | ($allowed | split(" ") | index($k)) == null)] | join(" ")' \
    "$BASEMAP_FILE")

  [[ -z "$unknown" ]] || die "basemap configuration: unknown importOptions: $unknown"

  for key in $IMPORT_OPTION_NUMBERS; do
    jq -e --arg k "$key" \
      'if (.importOptions // {} | has($k)) then ((.importOptions[$k] | type) == "number" and .importOptions[$k] >= 0 and .importOptions[$k] <= 20 and .importOptions[$k] == (.importOptions[$k] | floor)) else true end' \
      "$BASEMAP_FILE" >/dev/null \
      || die "basemap configuration: importOptions.$key must be a whole number between 0 and 20"
  done

  for key in $IMPORT_OPTION_STRINGS; do
    jq -e --arg k "$key" \
      'if (.importOptions // {} | has($k)) then ((.importOptions[$k] | type) == "string") else true end' \
      "$BASEMAP_FILE" >/dev/null \
      || die "basemap configuration: importOptions.$key must be a string"
  done

  for key in $IMPORT_OPTION_BOOLEANS; do
    jq -e --arg k "$key" \
      'if (.importOptions // {} | has($k)) then ((.importOptions[$k] | type) == "boolean") else true end' \
      "$BASEMAP_FILE" >/dev/null \
      || die "basemap configuration: importOptions.$key must be true or false"
  done

  for key in $IMPORT_OPTION_COUNTS; do
    jq -e --arg k "$key" \
      'if (.importOptions // {} | has($k)) then ((.importOptions[$k] | type) == "number" and .importOptions[$k] >= 0 and .importOptions[$k] == (.importOptions[$k] | floor)) else true end' \
      "$BASEMAP_FILE" >/dev/null \
      || die "basemap configuration: importOptions.$key must be a whole number"
  done

  check_option_order waterIndexMinMag waterIndexMaxMag
  check_option_order minIndexLevel maxIndexLevel
}

# A window whose lower bound is above its upper bound produces nothing useful,
# and the tools would only discover it after the download.
check_option_order()
{
  local lower upper min max

  lower="$1"
  upper="$2"

  min=$(jq -r --arg k "$lower" '.importOptions[$k] // empty' "$BASEMAP_FILE")
  max=$(jq -r --arg k "$upper" '.importOptions[$k] // empty' "$BASEMAP_FILE")

  if [[ -n "$min" && -n "$max" && "$min" -gt "$max" ]]; then
    die "basemap configuration: importOptions.$lower ($min) is greater than importOptions.$upper ($max)"
  fi
}

# --- configuration readers ----------------------------------------------

config_value()
{
  jq -r "$1" "$BASEMAP_FILE"
}

# Maps the configured options to the argument names of the two tools. The
# configuration is validated before this runs, so a key reaching a tool is a key
# that tool knows.
option_arguments()
{
  local key value

  import_arguments=()

  for key in $IMPORT_TOOL_OPTIONS; do
    value=$(jq -r --arg k "$key" '.importOptions[$k] // empty' "$BASEMAP_FILE")

    if [[ -n "$value" ]]; then
      import_arguments+=("--$key" "$value")
    fi
  done

  basemap_arguments=()

  for key in $BASEMAP_TOOL_OPTIONS; do
    value=$(jq -r --arg k "$key" '.importOptions[$k] // empty' "$BASEMAP_FILE")

    if [[ -n "$value" ]]; then
      basemap_arguments+=("--$key" "$value")
    fi
  done
}

coastlines_url()
{
  local url

  url=$(jq -r '.coastlines.url // empty' "$BASEMAP_FILE")

  if [[ -n "$url" ]]; then
    echo "$url"

    return
  fi

  [[ -n "$DEFAULT_COASTLINES_URL" ]] \
    || die "basemap configuration names no coastline location and the image carries no default"

  echo "$DEFAULT_COASTLINES_URL"
}

coastlines_sha256()
{
  jq -r '.coastlines.sha256 // empty' "$BASEMAP_FILE"
}

# --- script-owned state --------------------------------------------------

read_check_state()
{
  if [[ -f "$CHECK_STATE" ]]; then
    last_checked=$(jq -r '.lastCheckedAt // 0' "$CHECK_STATE")
    last_adopted=$(jq -r '.lastAdoptedAt // 0' "$CHECK_STATE")
    served_extract_md5=$(jq -r '.extractMd5 // ""' "$CHECK_STATE")
    served_coastlines_md5=$(jq -r '.coastlinesMd5 // ""' "$CHECK_STATE")
    coastlines_last_modified=$(jq -r '.coastlinesLastModified // ""' "$CHECK_STATE")
  else
    last_checked=0
    last_adopted=0
    served_extract_md5=""
    served_coastlines_md5=""
    coastlines_last_modified=""
  fi
}

# The check state is written when the decision of a pass is complete, never for
# an attempt: a failed production has to be due again on the next pass instead
# of waiting out the refresh window.
write_check_state()
{
  local extract_md5="$1" coastlines_md5="$2" last_modified="$3" adopted_at="$4"

  mkdir -p "$ADMIN_DIR"
  jq -n --argjson t "$(now_epoch)" \
        --argjson a "$adopted_at" \
        --arg e "$extract_md5" \
        --arg c "$coastlines_md5" \
        --arg lm "$last_modified" \
    '{lastCheckedAt: $t, lastAdoptedAt: $a, extractMd5: $e, coastlinesMd5: $c, coastlinesLastModified: $lm}' \
    > "$CHECK_STATE.tmp"
  mv "$CHECK_STATE.tmp" "$CHECK_STATE"
}

# --- downloads -----------------------------------------------------------

# Fetch the coastline archive with a conditional request: the source publishes
# no checksum and asks for If-Modified-Since instead of repeated downloads, so
# an unchanged remote copy is detected without transferring anything.
#
# On success the fetched copy is the candidate; whether it also becomes the
# adopted copy is decided by the caller.
fetch_coastlines()
{
  local url="$1" previous_last_modified="$2"
  local attempt=1 rc=0 code=""

  mkdir -p "$WORK_BASEMAP_DIR"

  while [[ "$attempt" -le "$DOWNLOAD_ATTEMPTS" ]]; do
    rm -f "$WORK_BASEMAP_DIR/coastlines.download"

    rc=0
    code=$(curl -fsSL --connect-timeout "$CONNECT_TIMEOUT" \
           ${previous_last_modified:+-z "$previous_last_modified"} \
           -D "$COASTLINES_HEADERS" \
           -o "$WORK_BASEMAP_DIR/coastlines.download" \
           -w '%{http_code}' "$url" 2>/dev/null) || rc=$?

    if [[ "$rc" = "0" ]]; then
      break
    fi

    log "coastline download attempt $attempt failed (curl status $rc)"

    if [[ "$attempt" -lt "$DOWNLOAD_ATTEMPTS" ]]; then
      sleep "$DOWNLOAD_RETRY_WAIT"
    fi

    attempt=$((attempt + 1))
  done

  [[ "$rc" = "0" ]] || return 1

  fetched_last_modified=$(grep -i '^last-modified:' "$COASTLINES_HEADERS" 2>/dev/null \
                          | tail -1 | sed 's/^[^:]*:[[:space:]]*//' | tr -d '\r' || true)

  # A conditional request that finds the remote copy unchanged transfers no
  # body: either the source says so, or the transfer produced nothing at all
  # (a source without last-modified information behaves this way).
  if [[ "$code" = "304" ]] || [[ ! -s "$WORK_BASEMAP_DIR/coastlines.download" ]]; then
    rm -f "$WORK_BASEMAP_DIR/coastlines.download"

    log "coastline source reports no change"

    [[ -f "$COASTLINES_CANDIDATE" ]] || return 1

    return 0
  fi

  mv "$WORK_BASEMAP_DIR/coastlines.download" "$COASTLINES_CANDIDATE"

  return 0
}

verify_coastlines_checksum()
{
  local expected="$1" actual

  [[ -n "$expected" ]] || return 0

  actual=$(sha256sum "$COASTLINES_CANDIDATE" | awk '{print $1}')

  if [[ "${actual,,}" != "${expected,,}" ]]; then
    error "coastline archive does not match the configured sha256"
    error "expected $expected, got $actual"

    rm -f "$COASTLINES_CANDIDATE"

    return 1
  fi

  return 0
}

unpack_coastlines()
{
  local shape

  rm -rf "$COASTLINES_DIR"
  mkdir -p "$COASTLINES_DIR"

  # unzip validates the CRC-32 of every entry, so a truncated download fails here
  if ! "$UNZIP_BIN" -o -q "$COASTLINES_ZIP" -d "$COASTLINES_DIR" 2>&1; then
    error "cannot unpack the coastline archive"

    return 1
  fi

  shape=$(find "$COASTLINES_DIR" -name '*.shp' -print -quit)

  [[ -n "$shape" ]] || { error "coastline archive contains no shape file"; return 1; }

  # The archive may already name the file the way the tool expects it, in which
  # case moving it onto itself would fail for no reason.
  if [[ "$shape" != "$COASTLINES_SHAPE" ]]; then
    mv "$shape" "$COASTLINES_SHAPE"
  fi

  return 0
}

# --- production ----------------------------------------------------------

# The import output is cached by the content of the planet export, so a pass
# that was triggered by new coastline data only runs the water index step.
import_extract()
{
  local extract="$1" extract_md5="$2"
  local cache_dir="$WORK_BASEMAP_DIR/import-$extract_md5"

  if [[ -f "$cache_dir/db.json" ]]; then
    log "reusing the import output of the current planet export"

    import_cache_dir="$cache_dir"

    return 0
  fi

  rm -rf "$cache_dir"
  mkdir -p "$cache_dir"

  log "importing the planet export (this is the expensive part)"

  if ! "$IMPORT_BIN" --typefile "$TYPEFILE" \
       --destinationDirectory "$cache_dir" \
       --source-url "file://$extract" --source-md5 "$extract_md5" \
       "${import_arguments[@]}" \
       "$extract" 2>&1 | tee "$WORK_BASEMAP_DIR/import.log"; then
    error "basemap import failed (see $WORK_BASEMAP_DIR/import.log)"

    return 1
  fi

  [[ -f "$cache_dir/db.json" ]] || { error "basemap import produced no db.json"; return 1; }

  import_cache_dir="$cache_dir"

  return 0
}

# The water index of a basemap comes from the world coastline data, not from the
# coastline ways of the planet export, so this runs after the import and
# replaces the index the import wrote.
add_water_index()
{
  log "generating the water index from the coastline data"

  if ! "$BASEMAP_IMPORT_BIN" --destinationDirectory "$OUT_DIR" \
       --coastlines "$COASTLINES_SHAPE" \
       "${basemap_arguments[@]}" 2>&1 | tee "$WORK_BASEMAP_DIR/waterindex.log"; then
    error "basemap water index generation failed (see $WORK_BASEMAP_DIR/waterindex.log)"

    return 1
  fi

  [[ -f "$OUT_DIR/water.idx" ]] || { error "basemap water index generation produced no water.idx"; return 1; }

  return 0
}

verify_file_set()
{
  local missing="" name

  for name in $REQUIRED_FILES; do
    if [[ ! -f "$OUT_DIR/$name" ]]; then
      missing="$missing $name"
    fi
  done

  if [[ -n "$missing" ]]; then
    error "produced basemap is missing files a client requires:$missing"

    return 1
  fi

  return 0
}

# --- placement -----------------------------------------------------------

# An earlier pass can have been interrupted between moving the served slot aside
# and moving the new one in. Close that gap before deciding anything else.
recover_staging()
{
  local old version target

  for old in "$STAGING_DIR"/v*.old; do
    [[ -d "$old" ]] || continue

    version=$(basename "$old" .old)
    target="$BASEMAP_PUBLIC_DIR/$version"

    if [[ -d "$target" ]]; then
      rm -rf "$old"
    else
      mkdir -p "$BASEMAP_PUBLIC_DIR"
      mv "$old" "$target"
      log "restored $version after an interrupted replacement"
    fi
  done

  rm -rf "$STAGING_DIR"/v*.new 2>/dev/null || true
}

place_slot()
{
  local version="$1" target new_dir name

  target="$BASEMAP_PUBLIC_DIR/v$version"
  new_dir="$STAGING_DIR/v$version.new"

  rm -rf "$new_dir"
  mkdir -p "$new_dir"

  while IFS= read -r name; do
    cp "$OUT_DIR/$name" "$new_dir/"
  done < <(jq -r '.output.files | keys[]' "$OUT_DIR/db.json")

  cp "$OUT_DIR/db.json" "$new_dir/"

  mkdir -p "$BASEMAP_PUBLIC_DIR"

  if [[ -d "$target" ]]; then
    mv "$target" "$STAGING_DIR/v$version.old"
  fi

  mv "$new_dir" "$target"
  rm -rf "$STAGING_DIR/v$version.old"

  log "placed basemap version $version"
}

write_manifest()
{
  local dir version changed entries="[]"

  for dir in $(ls -d "$BASEMAP_PUBLIC_DIR"/v* 2>/dev/null | sort -V || true); do
    version=$(basename "$dir")
    version="${version#v}"

    changed=$(jq -r '.generatedAt // empty' "$dir/db.json" 2>/dev/null || true)

    [[ -n "$changed" ]] || continue

    entries=$(jq -c --argjson v "$version" --arg c "$changed" \
              '. + [{typeConfigVersion: $v, changedAt: $c}]' <<<"$entries")
  done

  jq -n --argjson versions "$entries" '{schema: 1, versions: $versions}' > "$MANIFEST.tmp"
  mv "$MANIFEST.tmp" "$MANIFEST"

  log "wrote the availability manifest"
}

prune_slots()
{
  local history="$1" pruned="" kept

  if [[ "$history" -gt 0 ]]; then
    kept=$(ls -d "$BASEMAP_PUBLIC_DIR"/v* 2>/dev/null | sort -V | head -n -"$history" || true)
  else
    kept=""
  fi

  if [[ -n "$kept" ]]; then
    pruned=$(basename -a $kept 2>/dev/null | tr '\n' ' ' | sed 's/ $//')
    log "pruning basemap versions: $pruned"
    rm -rf $kept
  fi

  pruned_versions="$pruned"
}

prune_import_cache()
{
  local keep="$1" dir

  for dir in "$WORK_BASEMAP_DIR"/import-*; do
    [[ -d "$dir" ]] || continue

    if [[ "$dir" = "$keep" ]]; then
      continue
    fi

    log "removing the cached import output of an earlier planet export: $(basename "$dir")"
    rm -rf "$dir"
  done
}

write_generation_record()
{
  local version="$1" generated_at="$2" changed_at="$3" pruned="$4"

  mkdir -p "$ADMIN_DIR"
  jq -n --argjson v "$version" \
        --arg generatedAt "$generated_at" \
        --arg placedAt "$(date -u +%Y-%m-%dT%H:%M:%SZ)" \
        --arg changedAt "$changed_at" \
        --arg pruned "$pruned" \
    '{typeConfigVersion: $v, generatedAt: $generatedAt, changedAt: $changedAt, placedAt: $placedAt, pruned: ($pruned | split(" ") | map(select(. != "")))}' \
    > "$ADMIN_DIR/basemap_v${version}_generation.json.tmp"
  mv "$ADMIN_DIR/basemap_v${version}_generation.json.tmp" "$ADMIN_DIR/basemap_v${version}_generation.json"
}

# --- main ----------------------------------------------------------------

main()
{
  local refresh adoption history extract extract_md5 candidate_md5
  local url expected_sha256
  local last_checked last_adopted served_extract_md5 served_coastlines_md5 coastlines_last_modified
  local fetched_last_modified
  local extract_changed coastlines_changed adoption_due
  local version changed_at pruned_versions

  read_check_state

  validate_config

  mkdir -p "$BASEMAP_PUBLIC_DIR" "$ADMIN_DIR" "$STAGING_DIR" "$WORK_BASEMAP_DIR"

  recover_staging

  refresh=$(config_value '.refresh')
  adoption=$(config_value '.coastlinesRefresh')
  history=$(config_value '.history')

  if [[ $(( $(now_epoch) - last_checked )) -lt $(( refresh * 86400 )) ]]; then
    log "not due (last checked $last_checked, refresh $refresh days)"

    return 0
  fi

  extract=$(config_value '.extract')

  # Declared but unreadable input: a configuration failure, reported before any
  # import is attempted.
  [[ -f "$extract" ]] || die "planet export named by the basemap configuration cannot be read: $extract"

  log "checking the basemap inputs"

  extract_md5=$(md5sum "$extract" | awk '{print $1}')

  url=$(coastlines_url)
  expected_sha256=$(coastlines_sha256)
  fetched_last_modified="$coastlines_last_modified"

  if fetch_coastlines "$url" "$coastlines_last_modified"; then
    :
  elif [[ -f "$COASTLINES_CANDIDATE" ]]; then
    log "coastline source is not reachable, using the copy from the work area"
  else
    die "cannot fetch coastline data from $url"
  fi

  verify_coastlines_checksum "$expected_sha256" \
    || die "coastline data failed verification, the served basemap is left as it is"

  candidate_md5=$(md5sum "$COASTLINES_CANDIDATE" | awk '{print $1}')

  extract_changed=true
  if [[ -n "$served_extract_md5" ]] && [[ "$extract_md5" = "$served_extract_md5" ]]; then
    extract_changed=false
  fi

  coastlines_changed=true
  if [[ -n "$served_coastlines_md5" ]] && [[ "$candidate_md5" = "$served_coastlines_md5" ]]; then
    coastlines_changed=false
  fi

  adoption_due=true
  if [[ $(( $(now_epoch) - last_adopted )) -lt $(( adoption * 86400 )) ]]; then
    adoption_due=false
  fi

  if [[ "$extract_changed" = "false" ]] && [[ "$coastlines_changed" = "false" ]]; then
    log "no input changed"

    write_check_state "$extract_md5" "$served_coastlines_md5" "$fetched_last_modified" "$last_adopted"

    return 0
  fi

  if [[ "$extract_changed" = "false" ]] && [[ "$adoption_due" = "false" ]]; then
    log "newer coastline data is available but not due for adoption yet (last adopted $last_adopted, interval $adoption days)"

    write_check_state "$extract_md5" "$served_coastlines_md5" "$fetched_last_modified" "$last_adopted"

    return 0
  fi

  # Adopt the fetched copy when the coastline data changed and adoption is due,
  # and whenever there is no adopted copy to produce from at all. The fetched
  # copy is kept as well: it is what a later conditional request compares
  # against, so the next pass can be told "unchanged" without a transfer.
  if [[ "$coastlines_changed" = "true" ]] && { [[ "$adoption_due" = "true" ]] || [[ ! -f "$COASTLINES_ZIP" ]]; }; then
    cp "$COASTLINES_CANDIDATE" "$COASTLINES_ZIP"

    served_coastlines_md5="$candidate_md5"

    if [[ "$adoption_due" = "true" ]]; then
      last_adopted=$(now_epoch)
    fi

    log "adopted newer coastline data"
  elif [[ "$extract_changed" = "true" ]] && [[ "$coastlines_changed" = "true" ]]; then
    log "the planet export changed, the basemap is produced from the adopted coastline data"
  fi

  unpack_coastlines || die "cannot unpack the coastline data"

  option_arguments

  import_extract "$extract" "$extract_md5" || die "cannot produce the basemap database"

  rm -rf "$OUT_DIR"
  mkdir -p "$OUT_DIR"
  cp -a "$import_cache_dir/." "$OUT_DIR/"

  add_water_index || die "cannot produce the basemap water index"

  verify_file_set || die "the produced basemap is incomplete"

  version=$(jq -r '.typeConfigVersion' "$OUT_DIR/db.json")
  changed_at=$(jq -r '.generatedAt' "$OUT_DIR/db.json")

  place_slot "$version"

  # Pruning before the manifest keeps that manifest a description of what is
  # actually served: a version the manifest still named after it was pruned
  # would make a client download from a slot that is gone.
  prune_slots "$history"

  write_manifest

  write_generation_record "$version" "$(date -u +%Y-%m-%dT%H:%M:%SZ)" "$changed_at" "$pruned_versions"

  # The work area keeps the import output of the planet export that is now
  # served - it is what makes a coastline-only change cheap - and nothing else:
  # otherwise every new planet export would leave its import output behind.
  prune_import_cache "$import_cache_dir"

  write_check_state "$extract_md5" "$served_coastlines_md5" "$fetched_last_modified" "$last_adopted"

  log "done"

  return 0
}

case "${1:-}" in
  --help|-h)
    sed -n '2,46p' "$0" | sed 's/^# \{0,1\}//'
    exit 0
    ;;
  --check-config)
    validate_config
    echo "basemap config OK"
    exit 0
    ;;
  *)
    main
    ;;
esac
