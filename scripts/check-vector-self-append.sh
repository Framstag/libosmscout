#!/usr/bin/env bash
#
# Report appends that append an element of the sequence they append to.
#
# A sequence may move its storage when it grows, so an append that reads the element it stores
# through a reference into that same sequence reads an element the grow has already moved from or
# released: the appended value is then undefined, and a geometry silently loses a point or gains a
# garbage one. GCC reported the shape once ("possible uninitialized use" from
# `optimised.push_back(optimised.front())` in the geometry optimizer), but it reports neither every
# site nor the Meson build, so the shape is checked from the tree instead.
#
# The check is textual: it compares the receiver of the append with the leading expression of the
# argument. `nodeBuffer.push_back(ring->nodes[0])` names two different objects and is correct;
# `points.push_back(points[0])` does not.
#
# Usage:
#   scripts/check-vector-self-append.sh [path...]
#
# With no path it scans the repository's C++ sources (tracked and untracked, build directories
# excluded) and then verifies its own fixture under Tests/data/. With paths it scans exactly those
# files and directories, whatever their extension.
#
# Exit code: 0 when no self-append is found and the fixture behaves as expected, 1 when a
# self-append is found or the fixture check fails, 2 when a path is missing.

set -uo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"
fixture="$root/Tests/data/self-append-check/self-append.fixture"

# Directories that hold generated or vendored sources; the tree scan skips them.
skip_dirs='build|build-asan|build-meson|build-debug|debug|release|unity|android|subprojects'
source_extensions='*.cpp *.h *.hpp *.cc *.cxx *.mm *.inl *.tpp'

# Scans one file and prints "<file>:<line>: <receiver> <- <argument>" for every finding.
scan_file() {
  awk -v file="$1" '
    function is_receiver_char(c) {
      return c ~ /[A-Za-z0-9_.\[\]>-]/
    }

    {
      line = $0

      # The last append of the line, so that a nested call is reported at its own receiver.
      pos = 0
      for (i = 1; i <= length(line); i++) {
        if (substr(line, i, 10) == ".push_back")    { pos = i; name_len = 10 }
        else if (substr(line, i, 12) == ".emplace_back") { pos = i; name_len = 12 }
      }
      if (pos == 0) { next }

      # The append needs an argument list.
      j = pos + name_len
      while (j <= length(line) && substr(line, j, 1) ~ /[ \t]/) { j++ }
      if (substr(line, j, 1) != "(") { next }

      # The receiver: everything that forms one expression in front of the append.
      k = pos - 1
      receiver = ""
      while (k >= 1 && is_receiver_char(substr(line, k, 1))) {
        receiver = substr(line, k, 1) receiver
        k--
      }
      sub(/^[^A-Za-z_]+/, "", receiver)
      if (receiver == "") { next }

      argument = substr(line, j + 1)
      sub(/^[ \t]+/, "", argument)
      sub(/[ \t]+$/, "", argument)

      if (index(argument, receiver ".") == 1 ||
          index(argument, receiver "[") == 1 ||
          index(argument, receiver "->") == 1) {
        printf "%s:%d: %s <- %s\n", file, FNR, receiver, argument
      }
    }
  ' "$1"
}

# Prints the C++ sources of the repository, tracked and untracked, without the generated trees.
# A source tree without the repository metadata (a tarball, a container context) is walked instead.
tree_files() {
  if git -C "$root" rev-parse --is-inside-work-tree >/dev/null 2>&1; then
    git -C "$root" ls-files --cached --others --exclude-standard -- $source_extensions |
      grep -vE "^($skip_dirs)/"
  else
    find "$root" \( -type d \( -name .git -o -name 'build*' -o -name debug -o -name release \
                     -o -name unity -o -name android -o -name subprojects \) \) -prune -o \
         -type f \( -name '*.cpp' -o -name '*.h' -o -name '*.hpp' -o -name '*.cc' \
                     -o -name '*.cxx' -o -name '*.mm' -o -name '*.inl' -o -name '*.tpp' \) -print |
      sed "s#^$root/##"
  fi |
    sort -u
}

# Prints the files behind the given paths; directories are walked for C++ sources, files are used
# as they are so that a fixture of any extension can be scanned.
path_files() {
  for path in "$@"; do
    if [ -d "$path" ]; then
      find "$path" -type f | grep -E '\.(cpp|h|hpp|cc|cxx|mm|inl|tpp)$'
    elif [ -f "$path" ]; then
      printf '%s\n' "$path"
    else
      echo "check-vector-self-append: no such file or directory: $path" >&2
      return 2
    fi
  done
}

findings=""
checked=0

if [ $# -gt 0 ]; then
  if ! files="$(path_files "$@")"; then
    exit 2
  fi
else
  files="$(tree_files)"
fi

while IFS= read -r file; do
  [ -z "$file" ] && continue
  case "$file" in
    /*) ;;
    *) file="$root/$file" ;;
  esac
  checked=$((checked + 1))
  result="$(scan_file "$file")"
  if [ -n "$result" ]; then
    findings="$findings$result"$'\n'
  fi
done <<< "$files"

status=0

if [ -n "$findings" ]; then
  printf '%s' "$findings"
  count="$(printf '%s' "$findings" | grep -c .)"
  echo "check-vector-self-append: $count append(s) read an element of the sequence they append to" >&2
  status=1
fi

# The fixture proves the check still recognises the shape and still ignores a cross-container
# append. It is only part of the default run, not of an explicit path scan.
if [ $# -eq 0 ]; then
  if [ ! -f "$fixture" ]; then
    echo "check-vector-self-append: fixture missing: $fixture" >&2
    exit 2
  fi

  fixture_findings="$(scan_file "$fixture")"
  fixture_count="$(printf '%s' "$fixture_findings" | grep -c .)"

  if [ "$fixture_count" != "1" ] ||
     ! printf '%s' "$fixture_findings" | grep -q 'optimised <- optimised\.front()'; then
    echo "check-vector-self-append: the fixture check failed, expected exactly the self-append of the fixture:" >&2
    printf '%s\n' "$fixture_findings" >&2
    status=1
  fi

  if [ "$status" -eq 0 ]; then
    echo "check-vector-self-append: no self-append in $checked source file(s); fixture check passed."
  fi
fi

exit "$status"
