#!/bin/sh
# JavaScout launcher — detects build dir and runs with correct native library path
set -eu

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

# Default to project directory as working directory so relative stylesheet/icon
# paths in the FXML resolve correctly regardless of where the script is invoked.
cd "$PROJECT_DIR"

# --- Locate native library path ---
if [ -n "${JAVA_LIBRARY_PATH:-}" ]; then
    NATIVE_PATH="$JAVA_LIBRARY_PATH"
else
    for dir in "$PROJECT_DIR/build" "$PROJECT_DIR/build-meson" "$PROJECT_DIR/build-release" "$PROJECT_DIR/debug"; do
        # CMake layout: libosmscout-client-java/libosmscout_client_java.so
        candidate="$dir/libosmscout-client-java"
        if [ -f "$candidate/libosmscout_client_java.so" ]; then
            NATIVE_PATH="$candidate"
            break
        fi
        # Meson layout: libosmscout-client-java/src/libosmscout_client_java.so
        candidate="$dir/libosmscout-client-java/src"
        if [ -f "$candidate/libosmscout_client_java.so" ]; then
            NATIVE_PATH="$candidate"
            break
        fi
    done
fi

if [ -z "${NATIVE_PATH:-}" ] || [ ! -d "${NATIVE_PATH:-}" ]; then
    echo "Warning: native library path not found. Set JAVA_LIBRARY_PATH." >&2
fi

echo "[javascout.sh] native library path: ${NATIVE_PATH:-}" >&2

# --- Locate JavaScout jar (shaded = fat jar) ---
JAVASCOUT_JAR="$SCRIPT_DIR/target/javascout-1.0-SNAPSHOT.jar"
if [ ! -f "$JAVASCOUT_JAR" ]; then
    echo "Error: JavaScout jar not found. Build first: cd JavaScout && ./build.sh" >&2
    exit 1
fi

# --- Run (-D flags must come before -jar) ---
exec java \
     -Djava.library.path="$NATIVE_PATH" \
     -jar "$JAVASCOUT_JAR" \
     --stylesheet-dir "$PROJECT_DIR/stylesheets" \
     --icon-dir "$PROJECT_DIR/libosmscout/data/icons/14x14/standard"
