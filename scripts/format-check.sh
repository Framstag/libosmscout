#!/bin/bash
# format-check.sh - apply or verify code formatting with uncrustify
#
# Usage:
#   ./scripts/format-check.sh apply   # Format files in place
#   ./scripts/format-check.sh check   # Verify formatting (exit 1 if unformatted)
#   ./scripts/format-check.sh diff    # Show diff without modifying
#
# Requires: Uncrustify 0.83.0

set -uo pipefail

COMMAND="${1-help}"

if ! command -v uncrustify &>/dev/null; then
  echo "Error: uncrustify not found. Install Uncrustify 0.83.0." >&2
  exit 1
fi

PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
CONFIG="$PROJECT_ROOT/.uncrustify"

case "$COMMAND" in
  apply|check|diff) ;;
  *)
    echo "Usage: $0 {apply|check|diff}"
    echo ""
    echo "  apply   Format all tracked source files in place"
    echo "  check   Verify formatting (exit 0=clean, 1=unformatted)"
    echo "  diff    Show unified diff of pending changes"
    exit 1
    ;;
esac

# Find tracked source files, excluding non-owned directories
FILES=$(git -C "$PROJECT_ROOT" ls-files -- '*.h' '*.cpp' | \
  grep -v '^Android/' | \
  grep -v '^Apple/' | \
  grep -v '^subprojects/' || true)

if [ -z "$FILES" ]; then
  echo "No source files found." >&2
  exit 0
fi

case "$COMMAND" in
  apply)
    echo "Formatting with uncrustify..."
    while IFS= read -r file; do
      full_path="$PROJECT_ROOT/$file"
      if [ -f "$full_path" ]; then
        uncrustify -c "$CONFIG" --no-backup -l CPP -f "$full_path" -o "$full_path" 2>/dev/null || true
      fi
    done <<< "$FILES"
    echo "Formatted files: $(echo "$FILES" | wc -l)"
    ;;

  check)
    TMPDIR=$(mktemp -d)
    trap 'rm -rf "$TMPDIR"' EXIT
    HAD_ERRORS=0
    while IFS= read -r file; do
      full_path="$PROJECT_ROOT/$file"
      if [ ! -f "$full_path" ]; then
        continue
      fi
      formatted="$TMPDIR/$file"
      mkdir -p "$(dirname "$formatted")"
      uncrustify -c "$CONFIG" -l CPP -f "$full_path" 2>/dev/null > "$formatted" || true
      if ! diff -q "$full_path" "$formatted" >/dev/null 2>&1; then
        echo "Unformatted: $file" >&2
        HAD_ERRORS=1
      fi
    done <<< "$FILES"
    if [ "$HAD_ERRORS" -eq 0 ]; then
      echo "All files formatted correctly."
    else
      echo "Some files are not formatted correctly." >&2
    fi
    exit "$HAD_ERRORS"
    ;;

  diff)
    TMPDIR=$(mktemp -d)
    trap 'rm -rf "$TMPDIR"' EXIT
    while IFS= read -r file; do
      full_path="$PROJECT_ROOT/$file"
      if [ ! -f "$full_path" ]; then
        continue
      fi
      formatted="$TMPDIR/$file"
      mkdir -p "$(dirname "$formatted")"
      uncrustify -c "$CONFIG" -l CPP -f "$full_path" 2>/dev/null > "$formatted" || true
      if ! diff -q "$full_path" "$formatted" >/dev/null 2>&1; then
        diff -u "$full_path" "$formatted"
      fi
    done <<< "$FILES"
    ;;
esac
