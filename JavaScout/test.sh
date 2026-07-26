#!/usr/bin/env bash
# test.sh — run JavaScout Maven tests against a freshly built native client library.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="${1:-$PROJECT_DIR/build-gpx-on}"

if [ ! -d "$BUILD_DIR" ]; then
    echo "Build directory not found: $BUILD_DIR"
    echo "Usage: $0 [meson-build-dir]"
    echo "Build first: meson setup $BUILD_DIR -DenableGpx=true"
    exit 1
fi

BUILD_DIR="$(cd "$BUILD_DIR" && pwd)"

CLIENT_JAR="$BUILD_DIR/libosmscout-client-java/java/libosmscoutclientjava.jar"
CLIENT_SO="$BUILD_DIR/libosmscout-client-java/src/libosmscout_client_java.so.1.1.1"

if [ ! -f "$CLIENT_JAR" ] || [ ! -f "$CLIENT_SO" ]; then
    echo "Missing native client artifacts in $BUILD_DIR"
    echo "Build them with:"
    echo "  ninja -C $BUILD_DIR libosmscout-client-java/java/libosmscoutclientjava.jar libosmscout-client-java/src/libosmscout_client_java.so.1.1.1"
    exit 1
fi

echo "[test.sh] Installing libosmscout-client-java JAR to local Maven repository..."
mvn -q -f "$PROJECT_DIR/JavaScout/pom.xml" install:install-file \
    -Dfile="$CLIENT_JAR" \
    -DgroupId=net.sf.libosmscout \
    -DartifactId=libosmscout-client-java \
    -Dversion=1.0-SNAPSHOT \
    -Dpackaging=jar

LIB_DIR="$BUILD_DIR/libosmscout-client-java/src"
echo "[test.sh] Running mvn test with native library path: $LIB_DIR"
cd "$SCRIPT_DIR"
mvn test -Dnative.lib.dir="$LIB_DIR"
