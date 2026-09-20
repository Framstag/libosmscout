#!/bin/bash
# recovery-check-test.sh - does the regeneration script recover from failures?
#
# A deployment runs unattended: a schedule, no operator, months at a time. This
# check exercises the recovery paths without any real source data and without a
# network: the import tool is stubbed, so both outcomes (success and failure) can
# be produced on demand, and the source is a local file with its md5 sidecar.
#
# It runs inside the mapgen image (the CI smoke check) and outside it, on a
# machine with bash, curl, jq and md5sum, when MAPGEN_SCRIPT points at the script
# under test:
#
#   MAPGEN_CHECK_DIR=/tmp/check MAPGEN_SCRIPT=$PWD/scripts/mapgen/mapgen.sh \
#     bash scripts/mapgen/recovery-check-test.sh
#
# Environment:
#   MAPGEN_CHECK_DIR  fixture tree, must be writable (default: <work>/recovery-check)
#   MAPGEN_SCRIPT     the script under test (default: mapgen.sh from PATH)
#   MAPGEN_WORK_DIR   transient work area of the image (default: /work)
#
# Exits non-zero on the first case that does not behave as specified, printing
# the pass output that led to it.

set -u

WORK_ROOT="${MAPGEN_WORK_DIR:-/work}"
CHECK_DIR="${MAPGEN_CHECK_DIR:-${WORK_ROOT}/recovery-check}"
SCRIPT="${MAPGEN_SCRIPT:-mapgen.sh}"

CONFIG_DIR="${CHECK_DIR}/config"
REPO_DIR="${CHECK_DIR}/repo"
WORK_DIR="${CHECK_DIR}/work"
SOURCE_DIR="${CHECK_DIR}/source"
STUB_DIR="${CHECK_DIR}/stubs"

CHECK_STATE="${REPO_DIR}/private/admin/berlin_check.json"
PUBLIC_SLOT="${REPO_DIR}/public/europe/germany/berlin/v27"
DOWNLOADED="${WORK_DIR}/berlin.pbf"

failures=0
case_number=0

fail() {
  echo "  FAIL: $*" >&2
  [[ -n "${last_output:-}" ]] && echo "${last_output}" | sed 's/^/    | /' >&2
  failures=$((failures + 1))
  return 1
}

start_case() {
  case_number=$((case_number + 1))
  echo "case ${case_number}: $1"
}

# --- the fixture ---------------------------------------------------------

build_fixture() {
  rm -rf "${CHECK_DIR}"
  mkdir -p "${CONFIG_DIR}" "${REPO_DIR}/public" "${REPO_DIR}/private" "${WORK_DIR}" "${SOURCE_DIR}" "${STUB_DIR}"

  cat > "${REPO_DIR}/public/names.json" <<'JSON'
{"schema":1,"regions":[{"id":"europe","names":{"en":"Europe"},"children":[{"id":"germany","names":{"en":"Germany"},"children":[{"id":"berlin","names":{"en":"Berlin"}}]}]}]}
JSON

  cat > "${CONFIG_DIR}/imports.json" <<JSON
{"schema":1,"history":2,"refresh":7,"imports":[{"id":"berlin","url":"file://${SOURCE_DIR}/berlin.osm.pbf"}]}
JSON

  head -c 4096 /dev/urandom > "${SOURCE_DIR}/berlin.osm.pbf"
  # The sidecar is the fixture the script compares against, so md5 here is the
  # data being tested, not a security decision (see the justification for
  # shell:S4790 in sonar-project.properties and in scripts/mapgen/mapgen.sh).
  ( cd "${SOURCE_DIR}" && md5sum berlin.osm.pbf > berlin.osm.pbf.md5 )

  # a stub import tool, so a success and a failure can both be produced
  cat > "${STUB_DIR}/import-ok" <<'STUB'
#!/bin/bash
dest=""
while [ $# -gt 0 ]; do
  case "$1" in --destinationDirectory) dest="$2"; shift 2 ;; *) shift ;; esac
done
printf 'x' > "$dest/map.lib"
printf '{"typeConfigVersion":27,"generatedAt":"2026-01-01T00:00:00Z","output":{"files":{"map.lib":{"size":1,"crc32":1}}}}' > "$dest/db.json"
STUB
  cat > "${STUB_DIR}/import-fail" <<'STUB'
#!/bin/bash
echo "stub import: refusing to import" >&2
exit 1
STUB
  chmod +x "${STUB_DIR}/import-ok" "${STUB_DIR}/import-fail"
}

run_pass() {
  local import_tool="$1"
  last_output=$(MAPGEN_CONFIG_DIR="${CONFIG_DIR}" \
    MAPGEN_REPO_DIR="${REPO_DIR}" \
    MAPGEN_WORK_DIR="${WORK_DIR}" \
    MAPGEN_IMPORT="${STUB_DIR}/${import_tool}" \
    MAPGEN_DOWNLOAD_ATTEMPTS=2 \
    MAPGEN_DOWNLOAD_RETRY_WAIT=1 \
    "${SCRIPT}" 2>&1)
  return $?
}

expect_due() {
  run_pass "$1"
  echo "${last_output}" | grep -q "checking for changes"
}

expect_not_due() {
  run_pass "$1"
  echo "${last_output}" | grep -q "not due"
}

# --- the cases -----------------------------------------------------------

build_fixture

start_case "a failing import fails the run, records nothing and stays due"
if run_pass import-fail; then
  fail "a failing import must fail the run"
else
  [[ -f "${CHECK_STATE}" ]] && fail "a failed import must not record the check state"
  [[ -f "${DOWNLOADED}" ]] || fail "a failed import must keep the source for a retry"
  expect_due import-fail || fail "a failed import must stay due on the next run"
fi

start_case "the next run reuses the kept source, places, records and cleans up"
if run_pass import-ok; then
  echo "${last_output}" | grep -q "reusing the verified source" || fail "a kept source must be reused"
  [[ -f "${PUBLIC_SLOT}/map.lib" ]] || fail "the database was not placed"
  [[ -f "${CHECK_STATE}" ]] || fail "a completed import must record the check state"
  [[ -e "${DOWNLOADED}" ]] && fail "a successful run must remove the source"
  [[ -d "${WORK_DIR}/berlin-out" ]] && fail "a successful run must remove the import output"
else
  fail "the run that should place a database failed"
fi

start_case "a recorded check makes the same run not due"
expect_not_due import-ok || fail "a recorded check must make the import not due"

start_case "a source that fails verification is discarded"
rm -f "${CHECK_STATE}" "${PUBLIC_SLOT}"/* 2>/dev/null
printf '00000000000000000000000000000000  berlin.osm.pbf\n' > "${SOURCE_DIR}/berlin.osm.pbf.md5"
run_pass import-ok
echo "${last_output}" | grep -q "failed verification" || fail "an unverified download must be reported"
[[ -e "${DOWNLOADED}" ]] && fail "an unverified download must be discarded"
( cd "${SOURCE_DIR}" && md5sum berlin.osm.pbf > berlin.osm.pbf.md5 )

start_case "a download that fails is retried within the run"
chmod 000 "${SOURCE_DIR}/berlin.osm.pbf"
run_pass import-ok
chmod 644 "${SOURCE_DIR}/berlin.osm.pbf"
attempts=$(echo "${last_output}" | grep -c "download attempt")
[[ "${attempts}" -ge 1 ]] || fail "a failing download must be retried (saw ${attempts} attempts)"
echo "${last_output}" | grep -q "download failed after 2 attempts" || fail "the run must record the download as failed"

start_case "an interrupted replacement is rolled back"
rm -rf "${CHECK_DIR}"; build_fixture
run_pass import-ok > /dev/null || fail "the setup run failed"
mkdir -p "${REPO_DIR}/private/staging/berlin"
mv "${PUBLIC_SLOT}" "${REPO_DIR}/private/staging/berlin/v27.old"
printf '{"lastCheckedAt":%s,"lastSeenSourceMd5":""}\n' "$(date +%s)" > "${CHECK_STATE}"
run_pass import-ok
echo "${last_output}" | grep -q "restored" || fail "an interrupted replacement must be restored"
[[ -f "${PUBLIC_SLOT}/map.lib" ]] || fail "the restored database must be served again"
[[ -d "${REPO_DIR}/private/staging/berlin/v27.old" ]] && fail "the restored copy must not stay in staging"

# --- result --------------------------------------------------------------

echo
if [[ "${failures}" -gt 0 ]]; then
  echo "recovery check FAILED: ${failures} case(s)"
  exit 1
fi
echo "recovery check OK (${case_number} cases)"
