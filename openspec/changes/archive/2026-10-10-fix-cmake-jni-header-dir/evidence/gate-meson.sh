#!/bin/bash
# Meson half of the gate for fix-cmake-jni-header-dir.
# Meson does not read CMakeLists.txt, so this run is a parity gate: it proves the change broke nothing on
# the Meson side and records what the Meson JNI header generation does with the header directory.
cd /home/tim/projects/libosmscout || exit 1
echo "meson compile:"
meson compile -C build-meson
echo "meson compile rc=$?"
echo "meson test:"
meson test -C build-meson --timeout-multiplier 2 --print-errorlogs
echo "meson test rc=$?"
echo "--- Meson-generated JNI header directory of the java module:"
find build-meson/libosmscout-client-java -name "*.h" -path "*java*" 2>/dev/null | sort
