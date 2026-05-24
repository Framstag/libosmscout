#!/bin/bash
#
# Re-compute route descriptions from existing JSON files.
# Iterates JSON files in INPUT_DIR, extracts start/target coordinates,
# re-routes using LaneEvaluation describe, and saves to OUTPUT_DIR.
#
# Requires: jq (https://jqlang.github.io/jq/)
#
# Usage: recompute-routes.sh LANE_EVALUATION_BIN DATABASE INPUT_DIR OUTPUT_DIR

set -euo pipefail

if [ $# -ne 4 ]; then
    echo "Usage: $0 LANE_EVALUATION_BIN DATABASE INPUT_DIR OUTPUT_DIR"
    echo ""
    echo "  LANE_EVALUATION_BIN  Path to the LaneEvaluation executable"
    echo "  DATABASE             Path to the map database directory"
    echo "  INPUT_DIR            Directory with existing route JSON files"
    echo "  OUTPUT_DIR           Directory to write re-computed route JSON files"
    exit 1
fi

LANE_EVAL="$1"
DATABASE="$2"
INPUT_DIR="$3"
OUTPUT_DIR="$4"

if ! command -v jq &> /dev/null; then
    echo "ERROR: jq is required but not installed."
    echo "Install with: sudo apt install jq"
    exit 1
fi

if [ ! -x "$LANE_EVAL" ]; then
    echo "ERROR: LaneEvaluation binary not found or not executable: $LANE_EVAL"
    exit 1
fi

if [ ! -d "$INPUT_DIR" ]; then
    echo "ERROR: Input directory does not exist: $INPUT_DIR"
    exit 1
fi

mkdir -p "$OUTPUT_DIR"

success=0
fail=0
total=0

for json_file in "$INPUT_DIR"/*.json; do
    [ -f "$json_file" ] || continue
    total=$((total + 1))

    basename=$(basename "$json_file")

    start_lat=$(jq -r '.start.lat' "$json_file")
    start_lon=$(jq -r '.start.lon' "$json_file")
    target_lat=$(jq -r '.target.lat' "$json_file")
    target_lon=$(jq -r '.target.lon' "$json_file")

    echo "[$total] Re-routing $basename: ($start_lat, $start_lon) -> ($target_lat, $target_lon)"

    if "$LANE_EVAL" describe \
        --output "$OUTPUT_DIR/$basename" \
        "$DATABASE" "$start_lat" "$start_lon" "$target_lat" "$target_lon" \
        2>/dev/null; then
        success=$((success + 1))
    else
        echo "  FAILED"
        fail=$((fail + 1))
    fi
done

echo ""
echo "Done: $total routes processed, $success succeeded, $fail failed"
