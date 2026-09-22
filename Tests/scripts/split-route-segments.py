#!/usr/bin/env python3
#
# Split route-lane ground truth JSON files (as produced by `LaneEvaluation
# describe`) into short segments that each cover a window around one or more
# "interesting" junctions.
#
# Motivation: a full, long ground truth route is fragile against map updates
# - a re-imported map can shift the routed path or change the number of route
# description nodes anywhere along the route, which desynchronizes the
# position-based comparison done by `LaneEvaluation compare` even far away
# from the junction that actually matters. Splitting each route into short,
# junction-centered segments keeps each ground truth file focused on a single
# routing decision, making it much more likely to stay reproducible.
#
# A node counts as a junction if the route postprocessors attached a lane
# suggestion to it (`suggestedLanes`), or if it carries a real turn
# instruction (`turn` other than "Straight on"). For each junction, a window
# of `--segment-km` (default: 2.0, i.e. +/-1km) around its `distanceKm` is
# taken; overlapping/touching windows from nearby junctions are merged so
# they share one segment. Nodes outside every window are dropped - long
# straight stretches without any junction are not interesting for this
# dataset and are intentionally left out.
#
# Usage: split-route-segments.py [--segment-km KM] INPUT OUTPUT_DIR
#
#   INPUT       A route ground truth JSON file, or a directory of them.
#   OUTPUT_DIR  Directory to write the split segment JSON files into.
#
# This only slices the existing node data (rebasing distanceKm/nodeIndex) -
# it does not call the router. Review the output, then use
# recompute-routes.sh + `LaneEvaluation compare` against your database to
# confirm each split segment still reproduces before replacing the original
# file(s) in the ground truth dataset.

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path


def is_junction(node: dict) -> bool:
    if "suggestedLanes" in node:
        return True
    turn = node.get("turn")
    return turn is not None and turn != "Straight on"


def compute_segments(nodes: list[dict], segment_km: float) -> list[tuple[int, int]]:
    """Return (start_idx, end_idx) inclusive node ranges, one per merged
    junction window. Nodes outside every window are not covered by any
    range."""
    if not nodes:
        return []

    half = segment_km / 2.0
    total = nodes[-1]["distanceKm"]

    junction_indices = [i for i, n in enumerate(nodes) if is_junction(n)]
    if not junction_indices:
        return []

    windows = []
    for i in junction_indices:
        d = nodes[i]["distanceKm"]
        windows.append([max(0.0, d - half), min(total, d + half)])
    windows.sort()

    merged: list[list[float]] = []
    for w_start, w_end in windows:
        if merged and w_start <= merged[-1][1]:
            merged[-1][1] = max(merged[-1][1], w_end)
        else:
            merged.append([w_start, w_end])

    segments = []
    for w_start, w_end in merged:
        start_idx = next(i for i, n in enumerate(nodes) if n["distanceKm"] >= w_start)
        end_idx = max(i for i, n in enumerate(nodes) if n["distanceKm"] <= w_end)
        segments.append((start_idx, end_idx))
    return segments


def build_segment(route: dict, start_idx: int, end_idx: int) -> dict:
    nodes = route["nodes"][start_idx:end_idx + 1]
    base_distance = nodes[0]["distanceKm"]

    new_nodes = []
    for new_index, node in enumerate(nodes):
        new_node = dict(node)
        new_node["nodeIndex"] = new_index
        new_node["distanceKm"] = node["distanceKm"] - base_distance
        new_nodes.append(new_node)

    return {
        "version": route.get("version", 1),
        "database": route.get("database", ""),
        "start": {"lat": nodes[0]["lat"], "lon": nodes[0]["lon"]},
        "target": {"lat": nodes[-1]["lat"], "lon": nodes[-1]["lon"]},
        "nodes": new_nodes,
    }


def coord_to_filename_component(lat: float, lon: float) -> str:
    def part(value: float) -> str:
        sign = "n" if value < 0 else ""
        value = abs(value)
        int_part = int(value)
        frac_part = int((value - int_part) * 1_000_000)
        return f"{sign}{int_part}p{frac_part}"

    return f"{part(lat)}_{part(lon)}"


def route_filename(start_lat: float, start_lon: float, target_lat: float, target_lon: float) -> str:
    return (
        "route_"
        + coord_to_filename_component(start_lat, start_lon)
        + "__"
        + coord_to_filename_component(target_lat, target_lon)
        + ".json"
    )


def unique_output_path(output_dir: Path, filename: str, used: set[str]) -> Path:
    if filename not in used:
        used.add(filename)
        return output_dir / filename

    stem, suffix = filename[:-len(".json")], ".json"
    n = 2
    while f"{stem}-{n}{suffix}" in used:
        n += 1
    candidate = f"{stem}-{n}{suffix}"
    used.add(candidate)
    return output_dir / candidate


def split_file(input_path: Path, output_dir: Path, segment_km: float, used_names: set[str]) -> None:
    with input_path.open(encoding="utf-8") as f:
        route = json.load(f)

    nodes = route.get("nodes", [])
    segments = compute_segments(nodes, segment_km)

    if not segments:
        print(f"[{input_path.name}] no junctions found, skipping")
        return

    print(f"[{input_path.name}] {len(nodes)} nodes -> {len(segments)} segment(s)")

    for start_idx, end_idx in segments:
        segment = build_segment(route, start_idx, end_idx)
        length_km = nodes[end_idx]["distanceKm"] - nodes[start_idx]["distanceKm"]

        filename = route_filename(
            segment["start"]["lat"], segment["start"]["lon"],
            segment["target"]["lat"], segment["target"]["lon"],
        )
        out_path = unique_output_path(output_dir, filename, used_names)

        with out_path.open("w", encoding="utf-8") as f:
            json.dump(segment, f, indent=2, sort_keys=True, ensure_ascii=False)
            f.write("\n")

        note = ""
        if length_km > segment_km * 2:
            note = f"  WARNING: merged segment is {length_km:.2f}km, more than 2x --segment-km={segment_km}"
        print(f"  -> {out_path.name}  nodes {start_idx}-{end_idx}  ({length_km:.2f}km){note}")


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Split route-lane ground truth JSON files into short "
                     "segments that each cover a window around one or more junctions.")
    parser.add_argument("input", help="Route ground truth JSON file, or a directory of them")
    parser.add_argument("output", help="Output directory for the split segment files")
    parser.add_argument("--segment-km", type=float, default=2.0,
                        help="Target full segment length in km, i.e. +/-half around each junction (default: 2.0)")
    args = parser.parse_args()

    input_path = Path(args.input)
    output_dir = Path(args.output)

    if not input_path.exists():
        print(f"ERROR: input does not exist: {input_path}", file=sys.stderr)
        return 1

    if input_path.is_dir():
        input_files = sorted(input_path.glob("*.json"))
    else:
        input_files = [input_path]

    if not input_files:
        print(f"ERROR: no JSON files found in: {input_path}", file=sys.stderr)
        return 1

    output_dir.mkdir(parents=True, exist_ok=True)

    used_names: set[str] = set()
    for f in input_files:
        split_file(f, output_dir, args.segment_km, used_names)

    return 0


if __name__ == "__main__":
    sys.exit(main())
