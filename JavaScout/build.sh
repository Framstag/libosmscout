#!/bin/sh
# build.sh — build JavaScout: install client jar to local repo, then Maven package
set -eu

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

# Locate the Meson-built client jar if not given explicitly
if [ -n "${1:-}" ]; then
    CLIENT_JAR="$1"
else
    CLIENT_JAR=""
    for dir in "$PROJECT_DIR/debug" "$PROJECT_DIR/build" "$PROJECT_DIR/build-meson" "$PROJECT_DIR/build-release" "$PROJECT_DIR/build-cmake" "$PROJECT_DIR/build-release-cmake"; do
        # Meson layout: libosmscout-client-java/java/libosmscoutclientjava.jar
        candidate="$dir/libosmscout-client-java/java/libosmscoutclientjava.jar"
        if [ -f "$candidate" ]; then
            CLIENT_JAR="$candidate"
            break
        fi
        # CMake layout: libosmscout-client-java/libosmscoutclientjava.jar
        candidate="$dir/libosmscout-client-java/libosmscoutclientjava.jar"
        if [ -f "$candidate" ]; then
            CLIENT_JAR="$candidate"
            break
        fi
    done
fi

if [ -z "$CLIENT_JAR" ] || [ ! -f "$CLIENT_JAR" ]; then
    echo "Error: libosmscoutclientjava.jar not found." >&2
    echo "Build libosmscout-client-java first (CMake or Meson) or pass the jar path: $0 /path/to/libosmscoutclientjava.jar" >&2
    exit 1
fi

echo "[build.sh] installing client jar: $CLIENT_JAR" >&2

# Step 1: Install client jar to local Maven repository
mvn install:install-file \
  -Dfile="$CLIENT_JAR" \
  -DgroupId=net.sf.libosmscout \
  -DartifactId=libosmscout-client-java \
  -Dversion=1.0-SNAPSHOT \
  -Dpackaging=jar \
  -q

# Step 2: Build JavaScout
cd "$SCRIPT_DIR"
mvn package
